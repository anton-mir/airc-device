#include "uart_sensors.h"

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "string.h"
#include "semphr.h"

uint8_t spec_wake = '\n';
uint8_t spec_get_data = '\r';
uint8_t spec_sleep = 's';
uint8_t spec_continuous = 'c';

SemaphoreHandle_t HCHO_mutex = NULL;
SemaphoreHandle_t PMS_mutex = NULL;
SemaphoreHandle_t SPEC_mutex = NULL;

xTimerHandle timer_SPEC_ISR;

volatile char uart3IncomingDataBuffer[MAX_SPEC_BUF_LEN];
double pm2_5_val = 0;
double pm10_val = 0;
double HCHO_val = 0;

struct SPEC_values SPEC_SO2_values, SPEC_NO2_values, SPEC_CO_values, SPEC_O3_values;

const long long int SPEC_SO2_SN = 102219020326;
const long long int SPEC_NO2_SN = 31120010317;
const long long int SPEC_CO_SN = 60619020451;
const long long int SPEC_O3_SN = 22620010208;

typedef enum{
    SPEC_T_SO2,
    SPEC_T_NO2,
    SPEC_T_CO,
    SPEC_T_O3
}SPEC_TYPES;

typedef enum{
    HCHO_T,
    PM10_T,
    PM2_5_T,
}PM_HCHO_TYPES;

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
        {0,  1,  0,  0}, // 2 SO2 spec sensor TX
        {1,  1,  0,  0}, // 3 SO2 spec sensor RX
        {0,  0,  1,  0}, // 4 NO2 spec sensor TX
        {1,  0,  1,  0}, // 5 NO2 spec sensor RX
        {0,  1,  1,  0}, // 6 CO spec sensor  TX
        {1,  1,  1,  0}, // 7 CO spec sensor  RX
        {0,  0,  0,  1}, // 8 O3 spec sensor  TX
        {1,  0,  0,  1}, // 9 O3 spec sensor  RX
        {0,  1,  0,  1}, // 10
        {1,  1,  0,  1}, // 11
        {0,  0,  1,  1}, // 12
        {1,  0,  1,  1}, // 13
        {0,  1,  1,  1}, // 14
        {1,  1,  1,  1}  // 15
};

static void activate_multiplexer_channel(uint8_t channel){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, chan_table[channel][0]);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, chan_table[channel][1]);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, chan_table[channel][2]);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, chan_table[channel][3]);
}

static void get_spec_values(dataPacket_S *spec_struct, SemaphoreHandle_t mutex)
{
	if((mutex != NULL) && (xSemaphoreTake(mutex,portMAX_DELAY) == pdTRUE))
	{
        if (!SPEC_SO2_values.error_reason)
        {
            spec_struct->so2 = (double)SPEC_SO2_values.specPPB;
            spec_struct->so2_temp = (double)SPEC_SO2_values.specTemp;
            spec_struct->so2_hum = (double)SPEC_SO2_values.specRH;
        }
        else
        {
            spec_struct->so2 =
                    spec_struct->so2_temp =
                            spec_struct->so2_hum =
                                    (double)SPEC_SO2_values.error_reason;
        }

        if (!SPEC_NO2_values.error_reason)
        {
            spec_struct->no2 = (double) SPEC_NO2_values.specPPB;
            spec_struct->no2_temp = (double) SPEC_NO2_values.specTemp;
            spec_struct->no2_hum = (double) SPEC_NO2_values.specRH;
        }
        else
        {
            spec_struct->no2 =
                    spec_struct->no2_temp =
                            spec_struct->no2_hum =
                                    (double) SPEC_NO2_values.error_reason;
        }

        if (!SPEC_CO_values.error_reason)
        {
            spec_struct->co = (double) SPEC_CO_values.specPPB;
            spec_struct->co_temp = (double) SPEC_CO_values.specTemp;
            spec_struct->co_hum = (double) SPEC_CO_values.specRH;
        }
        else
        {
            spec_struct->co =
                    spec_struct->co_temp =
                            spec_struct->co_hum =
                                    (double) SPEC_CO_values.error_reason;
        }

        if (!SPEC_O3_values.error_reason)
        {
            spec_struct->o3 = (double) SPEC_O3_values.specPPB;
            spec_struct->o3_temp = (double) SPEC_O3_values.specTemp;
            spec_struct->o3_hum = (double) SPEC_O3_values.specRH;
        }
        else
        {
            spec_struct->o3 =
                    spec_struct->o3_temp =
                            spec_struct->o3_hum =
                                    (double) SPEC_O3_values.error_reason;
        }
    }
    xSemaphoreGive(mutex);
}

static double get_pm_hcho_values(SemaphoreHandle_t mutex,PM_HCHO_TYPES type){
	double res = 0;
	if((mutex != NULL) && 
	(xSemaphoreTake(mutex,portMAX_DELAY) == pdTRUE))
	{
		switch(type){
			case HCHO_T:
				res =  HCHO_val;
				break;
			case PM10_T:
				res = pm10_val;
				break;
			case PM2_5_T:
				res = pm2_5_val;
				break;
			default:
				break;
		}
		xSemaphoreGive(mutex);
	}
	return res;
}

void get_spec_sensors_data(dataPacket_S *spec_struct)
{
    get_spec_values(spec_struct, SPEC_mutex);
}

double get_pm2_5(void){
    return get_pm_hcho_values(PMS_mutex,PM2_5_T);
}

double get_pm10(void){
    return get_pm_hcho_values(PMS_mutex,PM10_T);
}

double get_HCHO(void){
   return get_pm_hcho_values(HCHO_mutex,HCHO_T);
}
void multiplexerSetState(uint8_t state)
{
    if (state)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    }
}

static void vTimerCallback_SPEC_ISR(xTimerHandle pxTimer) {
    BaseType_t *uart_rx_task_woken = pdFALSE;

    uint8_t data_left_in_dma_buffer = __HAL_DMA_GET_COUNTER(huart3.hdmarx);
    uint8_t dma_data_length = MAX_SPEC_BUF_LEN - data_left_in_dma_buffer;

    xTaskNotifyAndQueryFromISR(uart_sensors_handle, dma_data_length,
                               eSetValueWithOverwrite, NULL, &uart_rx_task_woken);

    xTimerStopFromISR( timer_SPEC_ISR, uart_rx_task_woken);
    portYIELD_FROM_ISR(uart_rx_task_woken);
}

HAL_StatusTypeDef getHCHO(uint8_t rx) {
    HAL_StatusTypeDef return_value = HAL_OK;

    multiplexerSetState(1);

    HAL_HalfDuplex_EnableReceiver(&huart3);

    activate_multiplexer_channel(rx);

    // Enable UART
    USART3->CR1 |= USART_CR1_UE;

    // Start DMA transfer and getting data
    const HAL_StatusTypeDef uart_receive_return = HAL_UART_Receive_DMA(&huart3, (unsigned char*)uart3IncomingDataBuffer, MAX_SPEC_BUF_LEN);

    // Pause task until all data received
        uint32_t dmaBufferLength = ulTaskNotifyTake(pdTRUE, (TickType_t) SDS011_RESPONSE_TIME);

    // Stop DMA transfer
    if (HAL_UART_DMAStop(&huart3) == HAL_ERROR)
    {
        return_value = HAL_ERROR;
    }

    // Disable UART
    USART3->CR1 &= ~USART_CR1_UE;

    multiplexerSetState(0);

    if (uart_receive_return == HAL_OK)
    {
        if (dmaBufferLength >= MIN_HCHO_BUF_LEN)
        {
            if (uart3IncomingDataBuffer[0] != HCHO_HEADER1 ||
                uart3IncomingDataBuffer[1] != HCHO_HEADER2 ||
                uart3IncomingDataBuffer[2] != HCHO_UNIT_PPB)
            {
                return_value = HAL_ERROR; // Got wrong data packet
            }
            else
            {
                if(xSemaphoreTake(HCHO_mutex,portMAX_DELAY) == pdTRUE){
                    HCHO_val = ((double) ((unsigned int) uart3IncomingDataBuffer[4] << 8 | uart3IncomingDataBuffer[5])) / 1000;
                    xSemaphoreGive(HCHO_mutex);
                }
            }
        }
    }
    else
    {
        return_value = HAL_ERROR;
    }

    memset((void*)uart3IncomingDataBuffer, '\0', MAX_SPEC_BUF_LEN);

    return return_value;
}

HAL_StatusTypeDef getSDS011(uint8_t rx) {
    HAL_StatusTypeDef return_value = HAL_OK;

    multiplexerSetState(1);

    HAL_HalfDuplex_EnableReceiver(&huart3);

    activate_multiplexer_channel(rx);

    // Enable UART
    USART3->CR1 |= USART_CR1_UE;

    // Start DMA transfer and getting data
    const HAL_StatusTypeDef uart_receive_return = HAL_UART_Receive_DMA(&huart3, (unsigned char*)uart3IncomingDataBuffer, MAX_SPEC_BUF_LEN);

    // Pause task until all data received
    uint32_t dmaBufferLength = ulTaskNotifyTake(pdTRUE, (TickType_t)SPEC_RESPONSE_TIME);

    // Stop DMA transfer
    if (HAL_UART_DMAStop(&huart3) == HAL_ERROR)
    {
        return_value = HAL_ERROR;
    }

    // Disable UART
    USART3->CR1 &= ~USART_CR1_UE;

    multiplexerSetState(0);

    if (uart_receive_return == HAL_OK)
    {
        if (dmaBufferLength >= MIN_SDS_BUF_LEN)
        {
            // packet format: AA C0 PM25_Low PM25_High PM10_Low PM10_High 0 0 CRC AB
            if (uart3IncomingDataBuffer[0] != SDS_HEADER1 ||
            uart3IncomingDataBuffer[1] != SDS_HEADER2 ||
            uart3IncomingDataBuffer[9] != SDS_TAIL)
            {
                return_value = HAL_ERROR; // error with packet
            }
            else
            {
                uint8_t crc_SDS = 0;
                for (int i = 0; i < 6; ++i)
                {
                    crc_SDS += uart3IncomingDataBuffer[i + 2];
                }
                if (crc_SDS != uart3IncomingDataBuffer[8])
                {
                    return_value = HAL_ERROR; //error with checksum
                }
                else
                {
                    if(xSemaphoreTake(PMS_mutex,portMAX_DELAY) == pdTRUE){
                        pm2_5_val = (double) ((int) uart3IncomingDataBuffer[2] | (int) (uart3IncomingDataBuffer[3] << 8)) / 10;
                        pm10_val = (double) ((int) uart3IncomingDataBuffer[4] | (int) (uart3IncomingDataBuffer[5] << 8)) / 10;
                        xSemaphoreGive(PMS_mutex);
                    }
                }
            }
        }
    }
    else
    {
        return_value = HAL_ERROR;
    }

    memset((void*)uart3IncomingDataBuffer, '\0', MAX_SPEC_BUF_LEN);

    return return_value;
}


HAL_StatusTypeDef getSPEC(uint8_t tx, uint8_t rx, struct SPEC_values *SPEC_gas_values, const long long int spec_sensor_sn)
{
    HAL_StatusTypeDef return_value = HAL_OK;
    SPEC_gas_values->error_reason = 0;

    multiplexerSetState(1); // Turn On multiplexer

    HAL_HalfDuplex_EnableTransmitter(&huart3);

    activate_multiplexer_channel(tx);

    // Enable UART
    USART3->CR1 |= USART_CR1_UE;

    if (HAL_UART_Transmit_IT(&huart3, &spec_wake, 1) != HAL_OK)
    {
        return_value = HAL_ERROR;
        SPEC_gas_values->error_reason = -1;
    }

    vTaskDelay((TickType_t)SPEC_RESPONSE_TIME);

    if (HAL_UART_Transmit_IT(&huart3, &spec_get_data, 1) != HAL_OK)
    {
        return_value = HAL_ERROR;
        SPEC_gas_values->error_reason = -2;
    }

    vTaskDelay((TickType_t)1);

    HAL_HalfDuplex_EnableReceiver(&huart3);

    // Activate data receive mode
    activate_multiplexer_channel(rx);

    // Start DMA transfer and getting data
    const HAL_StatusTypeDef uart_receive_return = HAL_UART_Receive_DMA(&huart3, (unsigned char*)uart3IncomingDataBuffer, MAX_SPEC_BUF_LEN);

    // Pause task until all data received
    uint32_t dmaBufferLength = ulTaskNotifyTake(pdTRUE, (TickType_t)SPEC_RESPONSE_TIME);

    // Stop DMA transfer
    if (HAL_UART_DMAStop(&huart3) == HAL_ERROR)
    {
        return_value = HAL_ERROR;
        SPEC_gas_values->error_reason = -3;
    }
    // Disable UART
    USART3->CR1 &= ~USART_CR1_UE;

    multiplexerSetState(0);

    // Check whether data was received
    if (uart_receive_return == HAL_OK)
    {
        // Check received data size
        if (dmaBufferLength < MIN_SPEC_BUF_LEN)
        {
            return_value = HAL_ERROR;
            SPEC_gas_values->error_reason = -4; // Probably need to adjust SPEC_NOTIFY_DELAY
        }
        else
        {
            char *pToNextValue;
            char *firstDeviderPtr;
            const char devider[] = ", ";

            // Check if SPEC sensor eeprom is not corrupted
            firstDeviderPtr = strstr((const char*)uart3IncomingDataBuffer, devider);
            int firstDeviderIndex = (int)(firstDeviderPtr - uart3IncomingDataBuffer);
            uint8_t specEepromCorrupted = firstDeviderIndex < 9; // Means there is no proper sensor ID in packet

            if (specEepromCorrupted)
            {
                return_value = HAL_ERROR;
                SPEC_gas_values->error_reason = -5; // Corrupted EEPROM - need to re-flash SPEC sensor EEPROM
            }
            else
            {
                SPEC_gas_values->specSN = strtoull((const char *) uart3IncomingDataBuffer, &pToNextValue, 10);

                if (SPEC_gas_values->specSN == spec_sensor_sn)
                {
                    if(xSemaphoreTake(SPEC_mutex, portMAX_DELAY) == pdTRUE)
                    {
                        SPEC_gas_values->specPPB = strtol(pToNextValue + 2, &pToNextValue, 10);
                        xSemaphoreGive(SPEC_mutex);
                    }
                    SPEC_gas_values->specTemp = strtoul(pToNextValue + 2, &pToNextValue, 10);
                    SPEC_gas_values->specRH = strtoul(pToNextValue + 2, &pToNextValue, 10);
                    pToNextValue = strstr(pToNextValue + 2, ", ");
                    pToNextValue = strstr(pToNextValue + 2, ", ");
                    pToNextValue = strstr(pToNextValue + 2, ", ");
                    SPEC_gas_values->specDay = strtoul(pToNextValue + 2, &pToNextValue, 10);
                    SPEC_gas_values->specHour = strtoul(pToNextValue + 2, &pToNextValue, 10);
                    SPEC_gas_values->specMinute = strtoul(pToNextValue + 2, &pToNextValue, 10);
                    SPEC_gas_values->specSecond = strtoul(pToNextValue + 2, NULL, 10);
                }
                else
                {
                    return_value = HAL_ERROR;
                    SPEC_gas_values->error_reason = -6; // Got wrong SPEC sensor ID
                }
            }
        }
    }
    else
    {
        return_value = HAL_ERROR;
        SPEC_gas_values->error_reason = -7; // HAL_UART_Receive_DMA got wrong return code
    }

    memset((void*)uart3IncomingDataBuffer, '\0', MAX_SPEC_BUF_LEN);

    return return_value;
}

static void UART_sensors_error_handler()
{
};

void uart_sensors(void * const arg) {

    /* Notify init task that UART sensors task has been started */
    xEventGroupSetBits(eg_task_started, EG_UART_SENSORS_STARTED);

    GPIO_Init();
    USART3_UART_Init();
    USART3_DMA_Init();
    HCHO_mutex = xSemaphoreCreateMutex();
    PMS_mutex = xSemaphoreCreateMutex();
    SPEC_mutex = xSemaphoreCreateMutex();
    timer_SPEC_ISR = xTimerCreate( "timer_SPEC_ISR",
                                   (TickType_t)SPEC_NOTIFY_DELAY, pdFALSE,
                                   (void*)0, vTimerCallback_SPEC_ISR);

    while (1) {

        if (getHCHO(MULTIPLEXER_CH0_HCHO_RX) != HAL_OK) {
            UART_sensors_error_handler();
        }
        if (getSDS011(MULTIPLEXER_CH1_SDS011_RX) != HAL_OK) {
            UART_sensors_error_handler();
        }

        if (getSPEC(MULTIPLEXER_CH2_SO2_TX, MULTIPLEXER_CH3_SO2_RX, &SPEC_SO2_values, SPEC_SO2_SN) != HAL_OK){
            UART_sensors_error_handler();
        }

        if (getSPEC(MULTIPLEXER_CH4_NO2_TX, MULTIPLEXER_CH5_NO2_RX, &SPEC_NO2_values, SPEC_NO2_SN) != HAL_OK){
            UART_sensors_error_handler();
        }

        if (getSPEC(MULTIPLEXER_CH6_CO_TX, MULTIPLEXER_CH7_CO_RX, &SPEC_CO_values, SPEC_CO_SN) != HAL_OK){
            UART_sensors_error_handler();
        }

        if (getSPEC(MULTIPLEXER_CH8_O3_TX, MULTIPLEXER_CH9_O3_RX, &SPEC_O3_values, SPEC_O3_SN) != HAL_OK){
            UART_sensors_error_handler();
        }

        vTaskDelay(500);
    }
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
void UART_SENSORS_IRQHandler(UART_HandleTypeDef *huart) {
    if (USART3 == huart->Instance)
    {
        configASSERT(uart_sensors_handle != NULL);
        // If UART is IDLE - no data at UART, so need to reset timer
        // Only after timer will occur we will count packet as received
        if(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) == SET)
        {
            __HAL_UART_CLEAR_IDLEFLAG(huart);
            BaseType_t uart_rx_task_woken = pdFALSE;
            xTimerResetFromISR( timer_SPEC_ISR, uart_rx_task_woken);
            xTimerStartFromISR( timer_SPEC_ISR, uart_rx_task_woken);
            portYIELD_FROM_ISR(uart_rx_task_woken);
        }
    }
}