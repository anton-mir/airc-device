#ifndef AIRC_DEVICE_UART_SENSORS_H
#define AIRC_DEVICE_UART_SENSORS_H

#include "stm32f4xx_hal.h"

UART_HandleTypeDef huart3;
DMA_HandleTypeDef huart3_dma_rx;

void UART_SENSORS_IRQHandler(UART_HandleTypeDef *huart);

void uart_sensors(void * const arg);
double get_SO2(void);
double get_NO2(void);
double get_CO(void);
double get_O3(void);
double get_pm2_5(void);
double get_pm10(void);
double get_HCHO(void);

static const uint8_t Sds011_WorkingMode[] = {
        0xAA,
        0xB4,
        0x04,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xFF,
        0xFF,
        0x06,
        0xAB
};


#endif //AIRC_DEVICE_UART_SENSORS_H
