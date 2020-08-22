#include "uart_sensors.h"

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "string.h"

#define  BUF_LEN   64
#define  TX_DELAY  100 / portTICK_RATE_MS
#define  RX_DELAY  3000 / portTICK_RATE_MS
#define SDS_HEADER1 0xAA
#define SDS_HEADER2 0xC0
#define SDS_TAIL 0xAB

volatile uint8_t rx;
volatile uint8_t command[BUF_LEN];
volatile uint8_t* SO2_data;
volatile uint8_t* NO2_data;
volatile uint8_t* CO_data;
volatile uint8_t* O3_data;
volatile uint8_t* SDS011_data;
volatile uint8_t* HCHO_data;
double SO2_val = 0;
double NO2_val = 0;
double CO_val = 0;
double O3_val = 0;
double pm2_5_val = 0;
double pm10_val = 0;
double HCHO_val = 0;

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

static HAL_StatusTypeDef USART3_DMA_Init(void)
{
    huart3_dma_rx.Instance = DMA1_Stream1;
    huart3_dma_rx.Init.Channel = DMA_CHANNEL_4;
    huart3_dma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    huart3_dma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    huart3_dma_rx.Init.MemInc = DMA_MINC_ENABLE;
    huart3_dma_rx.Init.Mode = DMA_NORMAL;
    huart3_dma_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    huart3_dma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    huart3_dma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    huart3_dma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&huart3_dma_rx) == HAL_ERROR) return HAL_ERROR;
    __HAL_LINKDMA(&huart3, hdmarx, huart3_dma_rx);

    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    if (HAL_UART_Receive_DMA(&huart3, command, BUF_LEN) == HAL_ERROR) return HAL_ERROR;

    return HAL_OK;
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

double get_pm2_5(void){
    return pm2_5_val;
}

double get_pm10(void){
    return pm10_val;
}

double get_HCHO(void){
    return HCHO_val;
}

static HAL_StatusTypeDef reset_dma_rx()
{
    if (HAL_UART_DMAStop(&huart3) == HAL_ERROR) return HAL_ERROR;
    if (HAL_UART_Receive_DMA(&huart3, command, BUF_LEN) == HAL_ERROR) return HAL_ERROR;

    return HAL_OK;
}

static uint8_t *check_uart_flag(uint8_t *flag)
{
    static uint32_t uart_notify;

    uart_notify = ulTaskNotifyTake(pdFALSE, RX_DELAY);
    if (!uart_notify) return NULL;

    return strstr(command, flag);
}

void uart_sensors(void * const arg) {

    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_UART_SENSORS_STARTED);

    GPIO_Init();
    USART3_UART_Init();
    USART3_DMA_Init();

    uint8_t spec_cmd = 'c';

    Set_chan(1);
    HAL_UART_Transmit_IT(&huart3, &Sds011_WorkingMode, 19);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    HAL_Delay(100);

    Set_chan(2);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    vTaskDelay(TX_DELAY);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(4);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    vTaskDelay(TX_DELAY);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(6);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    vTaskDelay(TX_DELAY);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    Set_chan(8);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
    vTaskDelay(TX_DELAY);
    HAL_UART_Transmit_IT(&huart3, &spec_cmd, 1);
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    while (1) {

        Set_chan(0);
        if (reset_dma_rx() == HAL_ERROR) continue;
        if (check_uart_flag('\n') != NULL) {
            strtod(command, &HCHO_data);
            HCHO_val = (double) ((int)HCHO_data[5] | (int)HCHO_data[4] << 8);
            memset(command, '\0', BUF_LEN);
        }

        Set_chan(1);
        if (reset_dma_rx() == HAL_ERROR) continue;
        if (check_uart_flag('\n') != NULL){
            // packet format: AA C0 PM25_Low PM25_High PM10_Low PM10_High 0 0 CRC AB
            if (command[0] != SDS_HEADER1 || command[1] != SDS_HEADER2 || command[9] != SDS_TAIL) {
                continue; // error with packet
            }
            uint8_t crc = 0;
            for (int i = 0; i < 6; ++i) {
                crc += command[i + 2];
            }
            if (crc != command[8]) {
                continue; //error with checksum
            }
            strtod(command, &SDS011_data);
            pm2_5_val = (double) ((int)SDS011_data[2] | (int)(SDS011_data[3] << 8)) / 10;
            pm10_val =  (double) ((int)SDS011_data[4] | (int)(SDS011_data[5] << 8)) / 10;
            memset(command, '\0', BUF_LEN);
        }


//        Set_chan(3);
//        if (reset_dma_rx() == HAL_ERROR) continue;
//        if (check_uart_flag('\n') != NULL){
//            strtod(command, &SO2_data);
//            SO2_val = strtod(strtok(SO2_data, "- ,"), NULL) / 100;
//            memset(command, '\0', BUF_LEN);
//        }
/*
        Set_chan(5);
        if (reset_dma_rx() == HAL_ERROR) continue;
        if (check_uart_flag('\n') != NULL){
            strtod(command, &NO2_data);
            NO2_val = strtod(strtok(NO2_data, "- ,"), NULL) / 100;
            memset(command, '\0', BUF_LEN);
        }

        Set_chan(7);
        if (reset_dma_rx() == HAL_ERROR) continue;
        if (check_uart_flag('\n') != NULL){
            strtod(command, &CO_data);
            CO_val = strtod(strtok(CO_data, "- ,"), NULL);
            memset(command, '\0', BUF_LEN);
        }

        Set_chan(9);
        if (reset_dma_rx() == HAL_ERROR) continue;
        if (check_uart_flag('\n') != NULL){
            strtod(command, &O3_data);
            O3_val = strtod(strtok(O3_data, "- ,"), NULL) / 100;
            memset(command, '\0', BUF_LEN);
        }*/

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
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0U);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
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

void UART_SENSORS_IRQHandler(UART_HandleTypeDef *huart)
{
    if (USART3 == huart->Instance)
    {
        configASSERT(uart_sensors_handle != NULL);

        if(RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
        {
            __HAL_UART_CLEAR_IDLEFLAG(huart);

            BaseType_t uart_rx_task_woken = pdFALSE;
            vTaskNotifyGiveFromISR(uart_sensors_handle, &uart_rx_task_woken);
            portYIELD_FROM_ISR(uart_rx_task_woken);
        }
    }
}