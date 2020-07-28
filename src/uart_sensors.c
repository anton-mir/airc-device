//
// Created by dmytro on 16.06.20.
//

#include "uart_sensors.h"

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "string.h"

volatile uint8_t rx;
volatile uint8_t command[64];
volatile uint8_t command_ready = 0;

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
    if (HAL_HalfDuplex_Init(&huart3) != HAL_OK){}
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

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

uint8_t chan_table[16][4] = {
        // s0, s1, s2, s3     channel
        {0,  0,  0,  0}, // 0 HCHO sensor RX
        {1,  0,  0,  0}, // 1 PM dust sensor RX
        {0,  1,  0,  0}, // 2 SO2 spec sensor RX
        {1,  1,  0,  0}, // 3 SO2 spec sensor TX
        {0,  0,  1,  0}, // 4 NO2 spec sensor RX
        {1,  0,  1,  0}, // 5 NO2 spec sensor TX
        {0,  1,  1,  0}, // 6 CO spec sensor RX
        {1,  1,  1,  0}, // 7 CO spec sensor TX
        {0,  0,  0,  1}, // 8 O3 spec sensor RX
        {1,  0,  0,  1}, // 9 O3 spec sensor TX
        {0,  1,  0,  1}, // 10
        {1,  1,  0,  1}, // 11
        {0,  0,  1,  1}, // 12
        {1,  0,  1,  1}, // 13
        {0,  1,  1,  1}, // 14
        {1,  1,  1,  1}  // 15
};

static void Set_chan(uint8_t channel){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, chan_table[channel][0]);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, chan_table[channel][1]);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, chan_table[channel][2]);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, chan_table[channel][3]);
}

void CO_sensor(void * const arg) {

    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_CO_SENSOR_STARTED);

    GPIO_Init();
    USART3_UART_Init();

    Set_chan(7);
    uint8_t spec_cmd = 'c';
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    HAL_Delay(100);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(6);
    /* Start reception once, rest is done in interrupt handler */
    HAL_UART_Receive_IT(&huart3, &rx, 1);
    while (1) {
        if (command_ready) {
            command_ready = 0;
        }
        vTaskDelay(500);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {

        BaseType_t reschedule = pdFALSE;
        UBaseType_t tx_uart_critical;

        tx_uart_critical = taskENTER_CRITICAL_FROM_ISR();

        vTaskNotifyGiveFromISR(CO_sensor_handle, &reschedule);
        portYIELD_FROM_ISR(reschedule);

        taskEXIT_CRITICAL_FROM_ISR( tx_uart_critical );
    }
}

    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        if (huart->Instance == USART3) {
            static uint8_t cmd[64];
            static uint8_t icmd;
            cmd[icmd] = rx;
            /* Parse received byte for EOL */
            if (rx == '\n') { /* If \r or \n print text */
                /* Terminate string with \0 */
                cmd[icmd] = 0;
                icmd = 0;
                strncpy(command, cmd, sizeof(command));
                command_ready = 1;
            } else if (rx == '\r') { /* Skip \r character */
            } else { /* If regular character, put it into cmd[] */
                cmd[icmd++] = rx;
            }
            /* Restart reception */
            HAL_UART_Receive_IT(&huart3, &rx, 1);
        }
     }

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(huart->Instance==USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();

        __HAL_RCC_GPIOD_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART3_IRQn, 5, 0U);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }

}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    if(huart->Instance==USART3)
    {
        __HAL_RCC_USART3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8);
        HAL_NVIC_DisableIRQ(USART3_IRQn);
    }

}