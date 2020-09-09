#ifndef AIRC_DEVICE_UART_SENSORS_H
#define AIRC_DEVICE_UART_SENSORS_H

#include "stm32f4xx_hal.h"

#define  MAX_SPEC_BUF_LEN 70
#define  MIN_SPEC_BUF_LEN 44

#define MULTIPLEXER_CH2_SO2_TX 2
#define MULTIPLEXER_CH3_SO2_RX 3
#define MULTIPLEXER_CH4_NO2_TX 4
#define MULTIPLEXER_CH5_NO2_RX 5
#define MULTIPLEXER_CH6_CO_TX  6
#define MULTIPLEXER_CH7_CO_RX  7
#define MULTIPLEXER_CH8_O3_TX  8
#define MULTIPLEXER_CH9_O3_RX  9

#define SPEC_RESPONSE_TIME 1000
#define SPEC_NOTIFY_DELAY 80

UART_HandleTypeDef huart3;
DMA_HandleTypeDef huart3_dma_rx;

void UART_SENSORS_IRQHandler(UART_HandleTypeDef *huart);

void uart_sensors(void * const arg);

struct SPEC_values
{
    unsigned long long int specSN;
    unsigned long int specPPB, specTemp, specRH, specDay, specHour, specMinute, specSecond;
};

struct SPEC_values* get_SO2(void);
struct SPEC_values* get_NO2(void);
struct SPEC_values* get_CO(void);
struct SPEC_values* get_O3(void);

#endif //AIRC_DEVICE_UART_SENSORS_H
