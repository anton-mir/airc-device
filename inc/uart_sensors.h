#ifndef AIRC_DEVICE_UART_SENSORS_H
#define AIRC_DEVICE_UART_SENSORS_H

#include "stm32f4xx_hal.h"

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
struct SPEC_values get_NO2(void);
struct SPEC_values get_CO(void);
struct SPEC_values get_O3(void);

HAL_StatusTypeDef getSPEC_SO2();


#endif //AIRC_DEVICE_UART_SENSORS_H
