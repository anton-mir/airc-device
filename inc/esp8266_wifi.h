#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include "stm32f4xx_hal.h"

#define ESP_UART_DELAY          1000
#define ESP_UART_BUFFER_SIZE    64

#define NUMBER_LENGTH(port) ((size_t)floor(log10(port) + 1))

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
    int ipd_flag;
    size_t tcp_length;
    uint8_t tcp_id;
    size_t tcp_counter;
    uint8_t tcp_data[];
} ESP8266_TCP_PACKET;

void wifi_task(void * const arg);
HAL_StatusTypeDef esp_module_init(void);
int search_in_buffer(uint8_t *buffer, size_t buffer_size, const uint8_t *needle, size_t needle_size);

void ESP_UART_DMAReceiveCpltCallback(DMA_HandleTypeDef *_hdma);

#endif /* ESP8266_WIFI_H */