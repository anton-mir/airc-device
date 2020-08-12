#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include "stm32f4xx_hal.h"

#define ESP_UART_DELAY          0x3E8  // (1000)
#define ESP_UART_BUFFER_SIZE    0x5C0  // (1472)
#define ESP_MAX_TCP_SIZE        0x800  // (2048)
#define ESP_INT_PRIO            0x8    // (9)

#define NUMBER_LENGTH(number) ((size_t)floor(log10(number) + 1))

typedef enum ESP8266_MODE {
    STA = 1,
    AP,
    STA_AP
} ESP8266_MODE;

typedef enum ESP8266_AP_ENC {
    OPEN = 0,
    WPA_PSK = 2,
    WPA2_PSK,
    WPA_WPA2_PSK
} ESP8266_AP_ENC;

struct ESP8266
{
    uint8_t initialized;
    ESP8266_MODE mode;
    char *sta_ssid, *sta_pass;
    char *ap_ssid, *ap_pass;
    uint8_t ap_chl;
    ESP8266_AP_ENC ap_enc;
    uint8_t mux;
    uint16_t port;
};

struct ESP8266_TCP_PACKET
{
    size_t length;
    uint8_t id;
    uint8_t *data;
};

void wifi_task(void * const arg);

void ESP_UART_IRQHandler(UART_HandleTypeDef *huart);

#endif /* ESP8266_WIFI_H */