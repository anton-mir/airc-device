
/*TODO: Fix possible wrong data income*/

#include "uart_sensors.h"

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "string.h"

#define  BUF_LEN 64

volatile uint8_t rx;
volatile uint8_t command[BUF_LEN];
volatile uint8_t* SO2_data;
volatile uint8_t* NO2_data;
volatile uint8_t* CO_data;
volatile uint8_t* O3_data;
double SO2_val = 0;
double NO2_val = 0;
double CO_val = 0;
double O3_val = 0;

static SemaphoreHandle_t rx_uart_sem = NULL;

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

static uint8_t chan_table[16][4] = {
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

double get_SO2(void){
    return SO2_val;
}

double get_NO2(void){
    return NO2_val;
}

double get_CO(void){
    return CO_val;
}

double get_O3(void){
    return O3_val;
}

void uart_sensors(void * const arg) {

    rx_uart_sem = xSemaphoreCreateBinary();
    configASSERT(rx_uart_sem != NULL);

    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_UART_SENSORS_STARTED);

    GPIO_Init();
    USART3_UART_Init();
    uint8_t spec_cmd = 'c';

    Set_chan(3);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    HAL_Delay(100);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(5);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    HAL_Delay(100);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(7);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    HAL_Delay(100);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(9);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    HAL_Delay(100);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    while (1) {

        Set_chan(2);
        HAL_UART_Receive_IT(&huart3, &rx, 1);
        if(xSemaphoreTake(rx_uart_sem, pdMS_TO_TICKS(3000)) == pdTRUE) {
            strtod(command, &SO2_data);
            SO2_val = strtod(strtok(SO2_data, "- ,"), NULL) / 100;
        }

        Set_chan(4);
        HAL_UART_Receive_IT(&huart3, &rx, 1);
        if(xSemaphoreTake(rx_uart_sem, pdMS_TO_TICKS(3000)) == pdTRUE) {
            strtod(command, &NO2_data);
            NO2_val = strtod(strtok(NO2_data, "- ,"), NULL) / 100;
        }

        Set_chan(6);
        HAL_UART_Receive_IT(&huart3, &rx, 1);
        if(xSemaphoreTake(rx_uart_sem, pdMS_TO_TICKS(3000)) == pdTRUE) {
            strtod(command, &CO_data);
            CO_val = strtod(strtok(CO_data, "- ,"), NULL);
        }

        Set_chan(8);
        HAL_UART_Receive_IT(&huart3, &rx, 1);
        if(xSemaphoreTake(rx_uart_sem, pdMS_TO_TICKS(3000)) == pdTRUE) {
            strtod(command, &O3_data);
            O3_val = strtod(strtok(O3_data, "- ,"), NULL) / 100;
        }

        vTaskDelay(500);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {

        BaseType_t reschedule = pdFALSE;

        vTaskNotifyGiveFromISR(uart_sensors_handle, &reschedule);
        portYIELD_FROM_ISR(reschedule);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {

        BaseType_t reschedule;

        UBaseType_t tx_uart_critical;
        tx_uart_critical = taskENTER_CRITICAL_FROM_ISR();

        static uint8_t cmd[64];
        static uint8_t icmd;
        cmd[icmd] = rx;

        /* Parse received byte for EOL */
        if (rx == '\n') { /* If \r or \n print text */
            /* Terminate string with \0 */
            cmd[icmd] = 0;
            icmd = 0;
            strncpy(command, cmd, sizeof(command));
            if (rx_uart_sem != NULL) {
                reschedule = pdFALSE;
                /* Unblock the task by releasing the semaphore.*/
                xSemaphoreGiveFromISR(rx_uart_sem, &reschedule);
                portYIELD_FROM_ISR(reschedule);
            }
        } else if (rx == '\r') { /* Skip \r character */
        } else { /* If regular character, put it into cmd[] */
            cmd[icmd++] = rx;
        }

        /* Restart reception */
        HAL_UART_Receive_IT(&huart3, &rx, 1);

        taskEXIT_CRITICAL_FROM_ISR( tx_uart_critical );
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