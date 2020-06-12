#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include "stm32f4xx_hal.h"

UART_HandleTypeDef esp_uart;

void wifi_task(void * const arg);
HAL_StatusTypeDef esp_module_init(void);

#endif /* ESP8266_WIFI_H */