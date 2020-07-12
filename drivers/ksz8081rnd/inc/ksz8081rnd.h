#ifndef KSZ8081RND_H
#define KSZ8081RND_H

#include "stm32f4xx_hal.h"

/* ################### Ethernet PHY GPIO configuration ###################### */

#define RMII_CSR_DV_PORT        GPIOA
#define RMII_CSR_DV_PIN         GPIO_PIN_7

#define RMII_PHYADD_PORT        GPIOA
#define RMII_PHYADD_PIN         GPIO_PIN_7

#define RMII_RXD0_PORT          GPIOC
#define RMII_RXD0_PIN           GPIO_PIN_4

#define RMII_RXD1_PORT          GPIOC
#define RMII_RXD1_PIN           GPIO_PIN_5

#define RMII_TXD0_PORT          GPIOB
#define RMII_TXD0_PIN           GPIO_PIN_12

#define RMII_TXD1_PORT          GPIOB
#define RMII_TXD1_PIN           GPIO_PIN_13

#define RMII_TXEN_PORT          GPIOB
#define RMII_TXEN_PIN           GPIO_PIN_11

#define RMII_MDIO_PORT          GPIOA
#define RMII_MDIO_PIN           GPIO_PIN_2

#define RMII_MDC_PORT           GPIOC
#define RMII_MDC_PIN            GPIO_PIN_1

#define RMII_REF_CLK_PORT       GPIOA
#define RMII_REF_CLK_PIN        GPIO_PIN_1

#define RMII_PHY_INT_PORT       GPIOE
#define RMII_PHY_INT_PIN        GPIO_PIN_8

#define RMII_PHY_RST_PORT       GPIOD
#define RMII_PHY_RST_PIN        GPIO_PIN_10

#define PHY_INT_PRIO            (7)

/* ################## Ethernet peripheral configuration ##################### */

/* Section 2: PHY configuration section */

/* KSZ8081RND PHY Address
 * Note: 0 - default address after startup */
#define PHY_ADDRESS                         0x03U
/* PHY Reset delay these values are based on a 1 ms Systick interrupt*/
#define PHY_RESET_DELAY                     0x000000FFU
/* PHY Configuration delay */
#define PHY_CONFIG_DELAY                    0x00000FFFU

#define PHY_READ_TO                         0x0000FFFFU
#define PHY_WRITE_TO                        0x0000FFFFU

/* Section 3: Common PHY Registers */

#define PHY_BASIC_CONTROL                   ((uint16_t)0x0000)
#define PHY_BASIC_STATUS                    ((uint16_t)0x0001)
#define PHY_IDENTIFIER1                     ((uint16_t)0x0002)
#define PHY_IDENTIFIER2                     ((uint16_t)0x0003)
#define PHY_AUTONEG_ADVERTISEMENT           ((uint16_t)0x0004)
#define PHY_AUTONEG_LINK_PARTNER_ABILITY    ((uint16_t)0x0005)
#define PHY_AUTONEG_EXPANSION               ((uint16_t)0x0006)
#define PHY_AUTONEG_NEXT_PAGE               ((uint16_t)0x0007)
#define PHY_LINK_PARTNER_NEXT_PAGE_ABILITY  ((uint16_t)0x0008)
#define PHY_DIGITAL_RESERVED_CONTROL        ((uint16_t)0x0010)
#define PHY_AFE_CONTROL1                    ((uint16_t)0x0011)
#define PHY_RX_ERROR_COUNTER                ((uint16_t)0x0015)
#define PHY_OPERATION_MODE_STRAP_OVERRIDE   ((uint16_t)0x0016)
#define PHY_OPERATION_MODE_STRAP_STATUS     ((uint16_t)0x0017)
#define PHY_EXPANDED_CONTROL                ((uint16_t)0x0018)
#define PHY_INTERRUPT_CONTROL               ((uint16_t)0x001B)
#define PHY_INTERRUPT_STATUS                ((uint16_t)0x001B)
#define PHY_LINKMD_CONTROL                  ((uint16_t)0x001D)
#define PHY_LINKMD_STATUS                   ((uint16_t)0x001D)
#define PHY_CONTROL1                        ((uint16_t)0x001E)
#define PHY_CONTROL2                        ((uint16_t)0x001F)

#define PHY_RESET                           ((uint16_t)0x8000)
#define PHY_LOOPBACK                        ((uint16_t)0x4000)
#define PHY_SPEED_100M                      ((uint16_t)0x2000)
#define PHY_SPEED_10M                       ((uint16_t)0x0000)
#define PHY_SPEED_MASK                      ((uint16_t)0x2000)
#define PHY_AUTONEGOTIATION_ENABLE          ((uint16_t)0x1000)
#define PHY_POWERDOWN                       ((uint16_t)0x0800)
#define PHY_ISOLATE                         ((uint16_t)0x0400)
#define PHY_RESTART_AUTONEGOTIATION         ((uint16_t)0x0200)
#define PHY_DUPLEX_FULL                     ((uint16_t)0x0100)
#define PHY_DUPLEX_HALF                     ((uint16_t)0x0000)
#define PHY_DUPLEX_MASK                     ((uint16_t)0x0100)

#define PHY_AUTONEG_COMPLETE                ((uint16_t)0x0020)
#define PHY_LINK_IS_UP                      ((uint16_t)0x0004)
#define PHY_JABBER_DETECTION                ((uint16_t)0x0002)

/* Section 4: Extended PHY Registers */

#define PHY_INTERRUPT_CONTROL               ((uint16_t)0x001B)
#define PHY_INTERRUPT_STATUS                ((uint16_t)0x001B)

#define PHY_LINK_STATUS                     ((uint16_t)0x0001)
#define PHY_FULL_DUPLEX                     ((uint16_t)0x0004)
#define PHY_SPEED_10BASE_T                  ((uint16_t)0x0001)
#define PHY_INT_LEVEL_ACTIVE_HIGH           ((uint16_t)0x0200)
#define PHY_INT_LEVEL_ACTIVE_LOW            ((uint16_t)0x0000)
#define PHY_INT_LEVEL_ACTIVE_MASK           ((uint16_t)0x0200)
#define PHY_LINK_DOWN_INT_EN                ((uint16_t)0x0400)
#define PHY_LINK_UP_INT_EN                  ((uint16_t)0x0100)
#define PHY_LINK_INT_UP_OCCURRED            ((uint16_t)0x0001)
#define PHY_LINK_INT_DOWN_OCCURED           ((uint16_t)0x0004)
#define PHY_REF_CLOCK_SELECT_MASK           ((uint16_t)0x0080)
#define PHY_REF_CLOCK_SELECT_25MHZ          ((uint16_t)0x0080)
#define PHY_REF_CLOCK_SELECT_50MHZ          ((uint16_t)0x0000)

typedef enum PHY_STATUS_ENUM
{
    E_PHY_STATUS_OK = 0,
    E_PHY_STATUS_ERROR,
    E_PHY_STATUS_TIMEOUT,
    E_PHY_STATUS_BUSY
} PHY_STATUS_E;

PHY_STATUS_E ksz8081_init(ETH_HandleTypeDef *heth, uint8_t *mac_addr);

#endif /* KSZ8081RND_H */

