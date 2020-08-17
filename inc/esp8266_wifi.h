#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include "stm32f4xx_hal.h"

#define ESP_UART_DELAY                 1000
#define ESP_UART_BUFFER_SIZE           2920
#define ESP_MAX_TCP_SIZE               2048
#define ESP_MAX_TCP_CONNECTIONS        5
#define ESP_INT_PRIO                   9

#define NUMBER_LENGTH(number) ((size_t)floor(log10(number) + 1))

typedef enum ESP8266_AP_ENC {
    OPEN = 0,
    WPA_PSK = 2,
    WPA2_PSK,
    WPA_WPA2_PSK
} ESP8266_AP_ENC;

struct ESP8266
{
    uint8_t initialized;
    char *sta_ssid, *sta_pass;
    size_t sta_ssid_size, sta_pass_size;
    char *ap_ssid, *ap_pass;
    uint8_t ap_chl;
    ESP8266_AP_ENC ap_enc;
    uint8_t ap_max_hosts;
    uint8_t ap_hidden_ssid;
};

struct ESP8266_TCP_PACKET
{
    size_t length;
    uint8_t id;
    uint8_t *data;
};

void wifi_task(void * const arg);
void ESP_InitPins(void);
HAL_StatusTypeDef ESP_InitUART(void);
HAL_StatusTypeDef ESP_InitDMA(void);
void ESP_UART_IRQHandler(UART_HandleTypeDef *huart);

#endif /* ESP8266_WIFI_H */