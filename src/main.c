/**
  ******************************************************************************
  * @file    Templates/Src/main.c
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "echo_server.h"
#include "ksz8081rnd.h"
#include "wh1602.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "tasks_def.h"
#include "hw_delay.h"

TaskHandle_t init_handle = NULL;
TaskHandle_t ethif_in_handle = NULL;
TaskHandle_t link_state_handle = NULL;
TaskHandle_t dhcp_fsm_handle = NULL;
TaskHandle_t echo_server_handle = NULL;

EventGroupHandle_t eg_task_started = NULL;

static RNG_HandleTypeDef rng_handle;

struct netif gnetif;

static void SystemClock_Config(void);
static void Error_Handler(void);
static void netif_setup();
void init_task(void *arg);

uint32_t rand_wrapper()
{
    uint32_t random = 0;
    
   (void)HAL_RNG_GenerateRandomNumber(&rng_handle, &random);

   return random;
}

void init_task(void *arg)
{
    GPIO_InitTypeDef gpio;
    BaseType_t status;
    struct netif *netif = (struct netif *)arg;

    __HAL_RCC_GPIOD_CLK_ENABLE();

    (void)HAL_RNG_Init(&rng_handle);

    eg_task_started = xEventGroupCreate();
    configASSERT(eg_task_started);

    xEventGroupSetBits(eg_task_started, EG_INIT_STARTED);

    /* Initialize LCD */
    lcd_init();
    lcd_clear();
    lcd_print_string("Initializing...");

    /* Create TCP/IP stack thread */
    tcpip_init(NULL, NULL);

    /* Initialize PHY */
    netif_setup();

    status = xTaskCreate(
                link_state,
                "link_st",
                LINK_STATE_TASK_STACK_SIZE,
                (void *)netif,
                LINK_STATE_TASK_PRIO,
                &link_state_handle);

    configASSERT(status);

    status = xTaskCreate(
                dhcp_fsm,
                "dhcp_fsm",
                DHCP_FSM_TASK_STACK_SIZE,
                (void *)netif,
                DHCP_FSM_TASK_PRIO,
                &dhcp_fsm_handle);

    configASSERT(status);

    status = xTaskCreate(
                ethernetif_input,
                "ethif_in",
                ETHIF_IN_TASK_STACK_SIZE,
                (void *)netif,
                ETHIF_IN_TASK_PRIO,
                &ethif_in_handle);

    configASSERT(status);

    status = xTaskCreate(
                echo_server,
                "echo_srv",
                ECHO_SERVER_TASK_STACK_SIZE,
                (void *)netif,
                ECHO_SERVER_TASK_PRIO,
                &echo_server_handle);

    configASSERT(status);

    /* Wait for all tasks initialization */
    xEventGroupWaitBits(
            eg_task_started,
            (EG_INIT_STARTED | EG_ETHERIF_IN_STARTED | EG_LINK_STATE_STARTED | 
                EG_DHCP_FSM_STARTED | EG_ECHO_SERVER_STARTED),
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);

    if (netif_is_up(netif))
    {
        /* Start DHCP address request */
        ethernetif_dhcp_start();
    }

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOD, &gpio);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);

    for (;;)
    {
        if (!netif_is_link_up(netif))
        {
            lcd_clear();
            lcd_print_string_at("Link:", 0, 0);
            lcd_print_string_at("down", 0, 1);
        }

        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
        vTaskDelay(500);
    }
}
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    BaseType_t status;

    HAL_Init();

    /* Configure the system clock to 168 MHz */
    SystemClock_Config();

    status = xTaskCreate(
                    init_task,
                    "init",
                    INIT_TASK_STACK_SIZE,
                    (void *)&gnetif,
                    INIT_TASK_PRIO,
                    &init_handle);

    configASSERT(status);

    vTaskStartScheduler();

    for (;;) { ; }
}

uint32_t HAL_GetTick(void)
{
    return xTaskGetTickCount();
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
        clocked below the maximum system frequency, to update the voltage scaling value
        regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
        clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
    if (HAL_GetREVID() == 0x1001)
    {
        /* Enable the Flash prefetch */
        __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    }
}

/**
  * @brief EXTI line detection callbacks
  * @param  pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == RMII_PHY_INT_PIN)
    {
        /* Get the IT status register value */
        ethernetif_phy_irq();
    }
}

static void netif_setup()
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    ip_addr_set_zero_ip4(&ipaddr);
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gw);

    /* Add the network interface */
    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

    /* Registers the default network interface */
    netif_set_default(&gnetif);

    /* Set the link callback function, this function is called on change of link status */
    netif_set_link_callback(&gnetif, ethernetif_link_update);

    if (netif_is_link_up(&gnetif))
    {
        /* When the netif is fully configured this function must be called */
        netif_set_up(&gnetif);
    }
    else
    {
        /* When the netif link is down this function must be called */
        netif_set_down(&gnetif);
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    /* User may add here some code to deal with this error */
    while(1)
    {
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
