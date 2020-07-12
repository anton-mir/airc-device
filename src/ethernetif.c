#include <string.h>
#include "ksz8081rnd.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "wh1602.h"
#include "main.h"

/* Network interface name */
#define IFNAME0 'G'
#define IFNAME1 'L'

#define DHCP_FSM_DELAY_MS           (500)
#define DHCP_MAX_TRIES              (3)

#define STRING_BUF_LEN              (21)

typedef enum DHCP_STATE_ENUM
{
    E_DHCP_OFF = 0,
    E_DHCP_START,
    E_DHCP_WAIT_ADDRESS,
    E_DHCP_ADDRESS_ASSIGNED,
    E_DHCP_TIMEOUT,
    E_DHCP_LINK_DOWN
} DHCP_STATE_E;

volatile ETH_HandleTypeDef h_eth;

/* Ethernet Receive Buffer */
__ALIGN_BEGIN uint8_t rx_buf[ETH_RX_BUF_NUM][ETH_RX_BUF_SIZE] __ALIGN_END;

/* Ethernet Rx DMA Descriptor */
__ALIGN_BEGIN ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RX_BUF_NUM] __ALIGN_END;

/* Ethernet Transmit Buffer */
__ALIGN_BEGIN uint8_t tx_buf[ETH_TX_BUF_NUM][ETH_TX_BUF_SIZE] __ALIGN_END;

/* Ethernet Tx DMA Descriptor */
__ALIGN_BEGIN ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TX_BUF_NUM] __ALIGN_END;

static SemaphoreHandle_t eth_irq_sem = NULL;
static SemaphoreHandle_t link_irq_sem = NULL;
static SemaphoreHandle_t dhcp_state_mut = NULL;
static volatile DHCP_STATE_E dhcp_state = E_DHCP_OFF;

static void low_level_init(struct netif *netif);
static err_t low_level_output(struct netif *netif, struct pbuf *p);
static struct pbuf * low_level_input(struct netif *netif);
static void ethernetif_notify_conn_changed(struct netif *netif);
static void ethernetif_rx_complete_cb(ETH_HandleTypeDef *heth);
static void dhcp_process(struct netif *netif);
static void dhcp_set_state(DHCP_STATE_E new_state);

/**
 * This function is called from the interrupt context
 */
static void ethernetif_rx_complete_cb(ETH_HandleTypeDef *heth)
{
    (void)heth;
    BaseType_t reschedule;

    if (eth_irq_sem != NULL)
    {
        reschedule = pdFALSE;
        /* Unblock the task by releasing the semaphore. */
        xSemaphoreGiveFromISR(eth_irq_sem, &reschedule);

        /* If reschedule was set to true we should yield. */
        portYIELD_FROM_ISR(reschedule);
    }
}

/**
  * @brief In this function, the hardware should be initialized.
  * Called from ethernetif_init().
  *
  * @param netif the already initialized lwip network interface structure
  *        for this ethernetif
  */
static void low_level_init(struct netif *netif)
{
    PHY_STATUS_E phy_status = E_PHY_STATUS_ERROR;
    uint8_t macaddress[6]= { MAC_ADDR0, MAC_ADDR1, MAC_ADDR2, MAC_ADDR3, MAC_ADDR4, MAC_ADDR5 };

    phy_status = ksz8081_init((ETH_HandleTypeDef *)&h_eth, macaddress);
    if (phy_status == E_PHY_STATUS_OK)
    {
        netif->flags |= NETIF_FLAG_LINK_UP;
    }

    /* Initialize Rx Descriptors list: Chain Mode  */
    HAL_ETH_DMARxDescListInit((ETH_HandleTypeDef *)&h_eth, dma_rx_desc_tab, &rx_buf[0][0], ETH_RX_BUF_NUM);

    /* Initialize Tx Descriptors list: Chain Mode */
    HAL_ETH_DMATxDescListInit((ETH_HandleTypeDef *)&h_eth, dma_tx_desc_tab, &tx_buf[0][0], ETH_RX_BUF_NUM);

    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    netif->hwaddr[0] = MAC_ADDR0;
    netif->hwaddr[1] = MAC_ADDR1;
    netif->hwaddr[2] = MAC_ADDR2;
    netif->hwaddr[3] = MAC_ADDR3;
    netif->hwaddr[4] = MAC_ADDR4;
    netif->hwaddr[5] = MAC_ADDR5;

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    /* Register TX completion callback */
    HAL_ETH_RegisterCallback(
        (ETH_HandleTypeDef *)&h_eth,
        HAL_ETH_RX_COMPLETE_CB_ID,
        ethernetif_rx_complete_cb);
    /* Enable MAC and DMA transmission and reception */
    HAL_ETH_Start((ETH_HandleTypeDef *)&h_eth);

    HAL_NVIC_SetPriority(ETH_IRQn, ETH_INT_PRIO, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
    * You can instead declare your own function an call etharp_output()
    * from it if you have to do some checks before sending (e.g. if link
    * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);


    return ERR_OK;
}

/**
  * @brief This function should do the actual transmission of the packet. The packet is
  * contained in the pbuf that is passed to the function. This pbuf
  * might be chained.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
  * @return ERR_OK if the packet could be sent
  *         an err_t value if the packet couldn't be sent
  *
  * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
  *       strange results. You might consider waiting for space in the DMA queue
  *       to become availale since the stack doesn't retry to send a packet
  *       dropped because of memory failure (except for the TCP timers).
  */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    err_t errval;
    struct pbuf *q;
    uint8_t *buffer = (uint8_t *)(h_eth.TxDesc->Buffer1Addr);
    __IO ETH_DMADescTypeDef *dma_tx_desc;
    uint32_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;

    dma_tx_desc = h_eth.TxDesc;
    bufferoffset = 0;

    /* copy frame from pbufs to driver buffers */
    for(q = p; q != NULL; q = q->next)
    {
        /* Is this buffer available? If not, goto error */
        if((dma_tx_desc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
        {
            errval = ERR_USE;
            goto error;
        }

        /* Get bytes in current lwIP buffer */
        byteslefttocopy = q->len;
        payloadoffset = 0;

        /* Check if the length of data to copy is bigger than Tx buffer size*/
        while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE)
        {
            /* Copy data to Tx buffer*/
            memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), (ETH_TX_BUF_SIZE - bufferoffset) );

            /* Point to next descriptor */
            dma_tx_desc = (ETH_DMADescTypeDef *)(dma_tx_desc->Buffer2NextDescAddr);

            /* Check if the buffer is available */
            if((dma_tx_desc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
            {
                errval = ERR_USE;
                goto error;
            }

            buffer = (uint8_t *)(dma_tx_desc->Buffer1Addr);

            byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
            payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
            framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
            bufferoffset = 0;
        }

        /* Copy the remaining bytes */
        memcpy((uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), byteslefttocopy);
        bufferoffset = bufferoffset + byteslefttocopy;
        framelength = framelength + byteslefttocopy;
    }

    /* Prepare transmit descriptors to give to DMA */
    HAL_ETH_TransmitFrame((ETH_HandleTypeDef *)&h_eth, framelength);

    errval = ERR_OK;

error:
    /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
    if ((h_eth.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET)
    {
        /* Clear TUS ETHERNET DMA flag */
        h_eth.Instance->DMASR = ETH_DMASR_TUS;

        /* Resume DMA transmission*/
        h_eth.Instance->DMATPDR = 0;
    }

    return errval;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf * low_level_input(struct netif *netif)
{
    struct pbuf *p = NULL;
    struct pbuf *q;
    uint16_t len;
    uint8_t *buffer;
    __IO ETH_DMADescTypeDef *dma_rx_desc;
    uint32_t bufferoffset = 0;
    uint32_t payloadoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t i = 0;

    if (HAL_ETH_GetReceivedFrame((ETH_HandleTypeDef *)&h_eth) != HAL_OK)
    {
        return NULL;
    }

    /* Obtain the size of the packet and put it into the "len" variable. */
    len = h_eth.RxFrameInfos.length;
    buffer = (uint8_t *)h_eth.RxFrameInfos.buffer;

    if (len > 0)
    {
        /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }

    if (p != NULL)
    {
        dma_rx_desc = h_eth.RxFrameInfos.FSRxDesc;
        bufferoffset = 0;

        for (q = p; q != NULL; q = q->next)
        {
            byteslefttocopy = q->len;
            payloadoffset = 0;

            /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size */
            while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE)
            {
                /* Copy data to pbuf */
                memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));

                /* Point to next descriptor */
                dma_rx_desc = (ETH_DMADescTypeDef *)(dma_rx_desc->Buffer2NextDescAddr);
                buffer = (uint8_t *)(dma_rx_desc->Buffer1Addr);

                byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
                payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
                bufferoffset = 0;
            }

            /* Copy remaining data in pbuf */
            memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), byteslefttocopy);
            bufferoffset = bufferoffset + byteslefttocopy;
        }
    }

    /* Release descriptors to DMA */
    /* Point to first descriptor */
    dma_rx_desc = h_eth.RxFrameInfos.FSRxDesc;
    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    for (i = 0; i < h_eth.RxFrameInfos.SegCount; i++)
    {
        dma_rx_desc->Status |= ETH_DMARXDESC_OWN;
        dma_rx_desc = (ETH_DMADescTypeDef *)(dma_rx_desc->Buffer2NextDescAddr);
    }

    /* Clear Segment_Count */
    h_eth.RxFrameInfos.SegCount =0;

    /* When Rx Buffer unavailable flag is set: clear it and resume reception */
    if ((h_eth.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)
    {
        /* Clear RBUS ETHERNET DMA flag */
        h_eth.Instance->DMASR = ETH_DMASR_RBUS;
        /* Resume DMA reception */
        h_eth.Instance->DMARPDR = 0;
    }

    return p;
}

void link_state(void * const arg)
{
    uint32_t regval = 0;
    struct netif *netif = (struct netif *)arg;

    link_irq_sem = xSemaphoreCreateBinary();
    configASSERT(link_irq_sem != NULL);

    xEventGroupSetBits(eg_task_started, EG_LINK_STATE_STARTED);

    for (;;)
    {
        if (xSemaphoreTake(link_irq_sem, portMAX_DELAY) == pdTRUE)
        {
            /* Read PHY_MISR*/
            HAL_ETH_ReadPHYRegister((ETH_HandleTypeDef *)&h_eth, PHY_INTERRUPT_STATUS, &regval);

            if (regval & PHY_LINK_INT_UP_OCCURRED)
            {
                netif_set_link_up(netif);
            }
            else if (regval & PHY_LINK_INT_DOWN_OCCURED)
            {
                netif_set_link_down(netif);
            }
        }
    }
}

static void dhcp_set_state(DHCP_STATE_E new_state)
{
    if (dhcp_state_mut != NULL)
    {
        xSemaphoreTake(dhcp_state_mut, portMAX_DELAY);
        dhcp_state = new_state;
        xSemaphoreGive(dhcp_state_mut);
    }
}

static void dhcp_process(struct netif *netif)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    struct dhcp *dhcp;
    char str[STRING_BUF_LEN] = { 0 };
  
    switch (dhcp_state)
    {
        case E_DHCP_START:
        {
            ip_addr_set_zero_ip4(&netif->ip_addr);
            ip_addr_set_zero_ip4(&netif->netmask);
            ip_addr_set_zero_ip4(&netif->gw);
            dhcp_start(netif);
            dhcp_set_state(E_DHCP_WAIT_ADDRESS);
            lcd_clear();
            lcd_print_string_at("DHCP:", 0, 0);
            lcd_print_string_at("starting...", 0, 1);
            break;
        }
    
        case E_DHCP_WAIT_ADDRESS:
        {
            if (dhcp_supplied_address(netif)) 
            {
                dhcp_set_state(E_DHCP_ADDRESS_ASSIGNED);

                sprintf(str, "%s",inet_ntoa(netif->ip_addr));
                lcd_clear();
                lcd_print_string_at("DHCP:", 0, 0);
                lcd_print_string_at(str, 0, 1);
            }
            else
            {
                dhcp = netif_dhcp_data(netif);
    
                /* DHCP timeout */
                if (dhcp->tries > DHCP_MAX_TRIES)
                {
                    dhcp_set_state(E_DHCP_TIMEOUT);
                    
                    /* Stop DHCP */
                    dhcp_stop(netif);
                    
                    /* Static address used */
                    IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
                    IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
                    IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
                    netif_set_addr(netif, &ipaddr, &netmask, &gw);
                    
                    sprintf(str, "%s",inet_ntoa(netif->ip_addr));
                    lcd_clear();
                    lcd_print_string_at("Static:", 0, 0);
                    lcd_print_string_at(str, 0, 1);
                }
                else
                {
                    sprintf(str, "retry: %d", dhcp->tries);
                    lcd_clear();
                    lcd_print_string_at("DHCP:", 0, 0);
                    lcd_print_string_at(str, 0, 1);
                }
            }
            break;
        }

        case E_DHCP_LINK_DOWN:
        {
            /* Stop DHCP */
            dhcp_stop(netif);
            dhcp_set_state(E_DHCP_OFF);
            break;
        }

        case E_DHCP_ADDRESS_ASSIGNED:
            /* no break here */
        case E_DHCP_TIMEOUT:
            /* no break here */
        case E_DHCP_OFF:
        {
            break;
        }

        default:
        {
            break;
        }
    }
}

void dhcp_fsm(void * const arg)
{
    struct netif *netif = (struct netif *)arg;

    dhcp_state_mut = xSemaphoreCreateMutex();
    configASSERT(dhcp_state_mut != NULL);

    /* Notify init task that DHCP task has been started */
    xEventGroupSetBits(eg_task_started, EG_DHCP_FSM_STARTED);

    for (;;)
    {
        dhcp_process(netif);
        vTaskDelay(DHCP_FSM_DELAY_MS);
    }
}

/**
  * @brief This function should be called when a packet is ready to be read
  * from the interface. It uses the function low_level_input() that
  * should handle the actual reception of bytes from the network
  * interface. Then the type of the received packet is determined and
  * the appropriate input function is called.
  *
  * @param netif the lwip network interface structure for this ethernetif
  */
void ethernetif_input(void * const arg)
{
    err_t err;
    struct netif *netif = (struct netif *)arg;
    struct pbuf *p;

    eth_irq_sem = xSemaphoreCreateBinary();
    configASSERT(eth_irq_sem != NULL);

    xEventGroupSetBits(eg_task_started, EG_ETHERIF_IN_STARTED);

    for (;;)
    {
        if (xSemaphoreTake(eth_irq_sem, portMAX_DELAY) == pdTRUE)
        {
            do
            {
                /* move received packet into a new pbuf */
                p = low_level_input(netif);

                if (p != NULL)
                {
                    /* entry point to the LwIP stack */
                    err = netif->input(p, netif);

                    if (err != ERR_OK)
                    {
                        pbuf_free(p);
                        p = NULL;
                    }
                }
            } while (p != NULL);
        }
    }
}

/**
  * @brief  This function sets the netif link status.
  * @param  netif: the network interface
  * @retval None
  */
void ethernetif_phy_irq()
{
    BaseType_t reschedule;

    if (link_irq_sem != NULL)
    {
        reschedule = pdFALSE;
        /* Unblock the task by releasing the semaphore. */
        xSemaphoreGiveFromISR(link_irq_sem, &reschedule);

        /* If reschedule was set to true we should yield. */
        portYIELD_FROM_ISR(reschedule);
    }
}

/**
  * @brief  Link callback function, this function is called on change of link status
  *         to update low level driver configuration.
  * @param  netif: The network interface
  * @retval None
  */
void ethernetif_link_update(struct netif *netif)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    if (netif_is_link_up(netif))
    {
        /* Restart the auto-negotiation */
        if (h_eth.Init.AutoNegotiation != ETH_AUTONEGOTIATION_DISABLE)
        {
            status = HAL_ETH_Autonegotiate((ETH_HandleTypeDef *)&h_eth);
            if (status != HAL_OK)
            {
                goto error;
            }
        }
        else /* AutoNegotiation Disable */
        {
error :
            /* Check parameters */
            assert_param(IS_ETH_SPEED(h_eth.Init.Speed));
            assert_param(IS_ETH_DUPLEX_MODE(h_eth.Init.DuplexMode));

            /* Set MAC Speed and Duplex Mode to PHY */
            (void)HAL_ETH_SetSpeedDuplex((ETH_HandleTypeDef *)&h_eth);
        }

        /* ETHERNET MAC Re-Configuration */
        HAL_ETH_ConfigMAC((ETH_HandleTypeDef *)&h_eth, (ETH_MACInitTypeDef *) NULL);

        /* Restart MAC interface */
        HAL_ETH_Start((ETH_HandleTypeDef *)&h_eth);
    }
    else
    {
        /* Stop MAC interface */
        HAL_ETH_Stop((ETH_HandleTypeDef *)&h_eth);
    }

    ethernetif_notify_conn_changed(netif);
}

/**
  * @brief  This function notify user about link status changement.
  * @param  netif: the network interface
  * @retval None
  */
static void ethernetif_notify_conn_changed(struct netif *netif)
{
    if (netif_is_link_up(netif))
    {
        netif_set_up(netif);
        dhcp_set_state(E_DHCP_START);
    }
    else
    {
        netif_set_down(netif);
        dhcp_set_state(E_DHCP_LINK_DOWN);
    }
}

void ethernetif_dhcp_start()
{
    dhcp_set_state(E_DHCP_START);
}

