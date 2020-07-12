//
// Created by anton on 12.07.20.
//

#include "uart_sensors.h"

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"


#define BUF_LEN                         (1024)
#define ECHO_SERVER_PORT                (11333)
#define MAX_CLIENTS                     (4)
static uint8_t buf[BUF_LEN];

//static void usart3_rx_complete_cb(ETH_HandleTypeDef *huar3);


static void USART3_UART_Init(void)
{
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 9600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_HalfDuplex_Init(&huart3) == HAL_OK)
    {
        strncpy(buf, "UART initialized\n", 17);
    }
}

static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6|GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

static void Set_CO_RX(void){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
}

static void Set_CO_TX(void){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
}

volatile uint8_t rx;
volatile uint8_t sensor_data_string[64];
volatile uint8_t got_sensor_data = 0;

void CO_sensor(void * const arg) {
    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_UART_SENSORS_STARTED);

    GPIO_Init();
    USART3_UART_Init();

    /* Register user RX completion callback
     * just to check how it works
     * need to enable USE_HAL_UART_REGISTER_CALLBACKS to use it*/
//    HAL_UART_RegisterCallback(
//            (UART_HandleTypeDef *)&huart3,
//            HAL_UART_RX_HALFCOMPLETE_CB_ID,
//            usart3_rx_complete_cb);

    HAL_NVIC_SetPriority(USART3_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    /* Start reception once, rest is done in interrupt handler */
    HAL_UART_Receive_IT(&huart3, &rx, 1);

    while (1) {
        if (got_sensor_data) {
            // TODO: Send sensor_data_string to main task later
            got_sensor_data = 0;
        }
        vTaskDelay(500);
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
    static uint8_t received_buffer[64];
    static uint8_t index = 0;
    received_buffer[index] = rx;

    /* Parse received byte for EOL */
    if (rx == '\n') { // End of transmission
        /* Terminate string with \0 */
        received_buffer[index] = 0;
        index = 0;
        strncpy(sensor_data_string, received_buffer, sizeof (sensor_data_string));
        got_sensor_data = 1;
    } else if (rx == '\r') { /* Skip \r character */
    } else { /* If regular character, put it into received_buffer[] */
        received_buffer[index++] = rx;
    }
    /* Restart reception */
    HAL_UART_Receive_IT(&huart3, &rx, 1);
}

/**
 * This function is called from the interrupt context
 * need to test that it works
 * maybe it is not needed because HAL_UART_RxHalfCpltCallback
 * can do all the job
 */
//static void usart3_rx_complete_cb(ETH_HandleTypeDef *huar3) {
//    (void) huar3;
//    vTaskDelay(1);
//}