#ifndef AIRC_DEVICE_UART_SENSORS_H
#define AIRC_DEVICE_UART_SENSORS_H

#include "stm32f4xx_hal.h"

UART_HandleTypeDef huart3;

void CO_sensor(void * const arg);
void echo_server(void * const arg);
void SDS_sensor(void * const arg);


typedef struct SDS_t {
    UART_HandleTypeDef* huart3;
    uint16_t pm_2_5;
    uint16_t pm_10;
    uint8_t data_receive[19];
} SDS;


void sdsInit(SDS* sds, const UART_HandleTypeDef* huart_sds);
void sds_uart_RxCpltCallback(SDS* sds, UART_HandleTypeDef *huart);

int8_t sdsSend(SDS* sds, const uint8_t *data_buffer, const uint8_t length);

uint16_t sdsGetPm2_5(SDS* sds);
uint16_t sdsGetPm10(SDS* sds);

static const uint8_t Sds011_SleepCommand[] = {
        0xAA,
        0xB4,
        0x06,
        0x01,
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
        0x05,
        0xAB
};

static const uint8_t Sds011_WorkingMode[] = {
        0xAA,
        0xB4,
        0x06,
        0x01,
        0x01,
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
