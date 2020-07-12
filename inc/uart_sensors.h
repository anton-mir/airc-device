#ifndef AIRC_DEVICE_UART_SENSORS_H
#define AIRC_DEVICE_UART_SENSORS_H

#include "stm32f4xx_hal.h"
//#include "stm32f4xx_hal_uart.h"

UART_HandleTypeDef huart3;

void CO_sensor(void * const arg);
void echo_server(void * const arg);

#endif //AIRC_DEVICE_UART_SENSORS_H
