#include <stdint.h>
#include <string.h>
#include "ksz8081rnd.h"
#include "hw_delay.h"

#define KSZ8081_RESET_ASSERT_DELAY_US   (500)
#define KSZ8081_BOOTUP_DELAY_US         (100)

static void ksz8081_gpio_init(ETH_HandleTypeDef *heth);
static int ksz8081_bootstrap();

/**
 * @brief PHY GPIO init
 *
 */
static void ksz8081_gpio_init(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef gpio;
    (void)heth;

    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    memset(&gpio, 0, sizeof(GPIO_InitTypeDef));

    gpio.Speed = GPIO_SPEED_HIGH;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF11_ETH;

    gpio.Pin = RMII_CSR_DV_PIN;
    HAL_GPIO_Init(RMII_CSR_DV_PORT, &gpio);

    gpio.Pin = RMII_RXD0_PIN;
    HAL_GPIO_Init(RMII_RXD0_PORT, &gpio);

    gpio.Pin = RMII_RXD1_PIN;
    HAL_GPIO_Init(RMII_RXD1_PORT, &gpio);

    gpio.Pin = RMII_TXD0_PIN;
    HAL_GPIO_Init(RMII_TXD0_PORT, &gpio);

    gpio.Pin = RMII_TXD1_PIN;
    HAL_GPIO_Init(RMII_TXD1_PORT, &gpio);

    gpio.Pin = RMII_TXEN_PIN;
    HAL_GPIO_Init(RMII_TXEN_PORT, &gpio);

    gpio.Pin = RMII_MDIO_PIN;
    HAL_GPIO_Init(RMII_MDIO_PORT, &gpio);

    gpio.Pin = RMII_MDC_PIN;
    HAL_GPIO_Init(RMII_MDC_PORT, &gpio);

    gpio.Pin = RMII_REF_CLK_PIN;
    HAL_GPIO_Init(RMII_REF_CLK_PORT, &gpio);

    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = RMII_PHY_INT_PIN;
    HAL_GPIO_Init(RMII_PHY_INT_PORT, &gpio);

    /* Enable and set EXTI Line 8 Interrupt */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, PHY_INT_PRIO, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static int ksz8081_bootstrap()
{
    GPIO_InitTypeDef gpio;

    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    memset(&gpio, 0, sizeof(GPIO_InitTypeDef));

    gpio.Speed = GPIO_SPEED_MEDIUM;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = RMII_PHY_RST_PIN;
    HAL_GPIO_Init(RMII_PHY_RST_PORT, &gpio);

    gpio.Speed = GPIO_SPEED_MEDIUM;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = RMII_CSR_DV_PIN;
    HAL_GPIO_Init(RMII_CSR_DV_PORT, &gpio);

    /* Reset PHY */
    HAL_GPIO_WritePin(RMII_PHY_RST_PORT, RMII_PHY_RST_PIN, GPIO_PIN_RESET);
    /* Set PHY address to 0x03 */
    HAL_GPIO_WritePin(RMII_CSR_DV_PORT, RMII_CSR_DV_PIN, GPIO_PIN_SET);
    /* Reset pin should be asserted for minimum 500 us */
    delay_us(KSZ8081_RESET_ASSERT_DELAY_US);
    /* Bootup PHY */
    HAL_GPIO_WritePin(RMII_PHY_RST_PORT, RMII_PHY_RST_PIN, GPIO_PIN_SET);
    /* Bootup delay should be minimum 100 us */
    delay_us(KSZ8081_BOOTUP_DELAY_US);

    HAL_GPIO_DeInit(RMII_CSR_DV_PORT, RMII_CSR_DV_PIN);

    return 0;
}

static PHY_STATUS_E ksz8081_hal_error_map(HAL_StatusTypeDef err)
{
    PHY_STATUS_E retval = E_PHY_STATUS_ERROR;

    switch (err)
    {
        case HAL_OK:
        {
            retval = E_PHY_STATUS_OK;
            break;
        }

        case HAL_ERROR:
        {
            retval = E_PHY_STATUS_ERROR;
            break;
        }

        case HAL_BUSY:
        {
            retval = E_PHY_STATUS_BUSY;
            break;
        }

        case HAL_TIMEOUT:
        {
            retval = E_PHY_STATUS_TIMEOUT;
            break;
        }

        default:
        {
            retval = E_PHY_STATUS_ERROR;
            break;
        }
    }

    return retval;
}

PHY_STATUS_E ksz8081_init(ETH_HandleTypeDef *heth, uint8_t *mac_addr)
{
    int ksz_status = 0;
    HAL_StatusTypeDef hal_status = HAL_ERROR;
    uint32_t regval = 0;

    if (heth == NULL || mac_addr == NULL)
    {
        return E_PHY_STATUS_ERROR;
    }

    ksz_status = ksz8081_bootstrap();
    if (ksz_status < 0)
    {
        return E_PHY_STATUS_ERROR;
    }

    hal_status = HAL_ETH_RegisterCallback(
                    heth,
                    HAL_ETH_MSPINIT_CB_ID,
                    ksz8081_gpio_init);
    if (hal_status != HAL_OK)
    {
        return E_PHY_STATUS_ERROR;
    }

    /* Enable ETHERNET clock  */
    __HAL_RCC_ETH_CLK_ENABLE();

    heth->Instance = ETH;
    heth->Init.MACAddr = mac_addr;
    heth->Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    heth->Init.Speed = ETH_SPEED_100M;
    heth->Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    heth->Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
    heth->Init.RxMode = ETH_RXINTERRUPT_MODE;
    heth->Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
    heth->Init.PhyAddress = PHY_ADDRESS;

    /* configure ethernet peripheral */
    hal_status = HAL_ETH_Init(heth);


    /**** Configure PHY to generate an interrupt when Eth Link state changes ****/
    HAL_ETH_ReadPHYRegister(heth, PHY_CONTROL2, &regval);
    regval &= ~(PHY_INT_LEVEL_ACTIVE_MASK);
    regval |= PHY_INT_LEVEL_ACTIVE_LOW;
    HAL_ETH_WritePHYRegister(heth, PHY_CONTROL2, regval);

    /* Read Register Configuration */
    HAL_ETH_ReadPHYRegister(heth, PHY_INTERRUPT_CONTROL, &regval);

    regval |= (PHY_LINK_UP_INT_EN | PHY_LINK_DOWN_INT_EN);

    /* Enable Interrupt on change of link status */
    HAL_ETH_WritePHYRegister(heth, PHY_INTERRUPT_CONTROL, regval);
    return ksz8081_hal_error_map(hal_status);
}
