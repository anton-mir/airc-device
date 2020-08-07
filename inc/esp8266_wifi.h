#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include "stm32f4xx_hal.h"

#define ESP_UART_DELAY          0x3E8  // (1000)
#define ESP_UART_BUFFER_SIZE    0x40   // (64)
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

typedef struct ESP8266
{
    uint8_t initialized;
    ESP8266_MODE mode;
    char *sta_ssid, *sta_pass;
    char *ap_ssid, *ap_pass;
    uint8_t ap_chl;
    ESP8266_AP_ENC ap_enc;
    uint8_t mux;
    uint16_t port;
} ESP8266;

typedef struct ESP8266_TCP_PACKET
{
    size_t length;
    size_t buffer_size;
    uint8_t id;
    uint8_t *data;
} ESP8266_TCP_PACKET;

typedef struct ESP8266_TCP_QUAUE
{
    size_t count;
    ESP8266_TCP_PACKET *packets;
} ESP8266_TCP_QUAUE;

void wifi_task(void * const arg);
HAL_StatusTypeDef esp_module_init(void);

void ESP_UART_DMAReceiveCpltCallback(DMA_HandleTypeDef *_hdma);

#endif /* ESP8266_WIFI_H */