#ifndef AIRC_DEVICE_UART_SENSORS_H
#define AIRC_DEVICE_UART_SENSORS_H

#include "stm32f4xx_hal.h"

#define  MAX_SPEC_BUF_LEN 70
#define  MIN_SPEC_BUF_LEN 47

#define MULTIPLEXER_CH0_HCHO_RX 0
#define MULTIPLEXER_CH1_SDS011_RX 1
#define MULTIPLEXER_CH2_SO2_TX 2
#define MULTIPLEXER_CH3_SO2_RX 3
#define MULTIPLEXER_CH4_NO2_TX 4
#define MULTIPLEXER_CH5_NO2_RX 5
#define MULTIPLEXER_CH6_CO_TX  6
#define MULTIPLEXER_CH7_CO_RX  7
#define MULTIPLEXER_CH8_O3_TX  8
#define MULTIPLEXER_CH9_O3_RX  9

#define SPEC_RESPONSE_TIME 1000

#define SDS_HEADER1 0xAA
#define SDS_HEADER2 0xC0
#define SDS_TAIL 0xAB

#define HCHO_STARTBYTE 0XFF

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
