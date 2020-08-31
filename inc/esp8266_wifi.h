#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#define ESP_UART_DELAY                 1000
#define ESP_UART_BUFFER_SIZE           1472
#define ESP_MAX_TCP_SIZE               2048
#define ESP_MAX_TCP_CONN               5
#define ESP_INT_PRIO                   9

#define NUMBER_LENGTH(number) ((size_t)floor(log10(number) + 1))

typedef enum ESP8266_AP_ENC 
{
    OPEN = 0,
    WPA_PSK = 2,
    WPA2_PSK,
    WPA_WPA2_PSK
} ESP8266_AP_ENC;

typedef enum ESP8266_SERVER_HANDLERS 
{
    ESP_VOID_HANDLER,
    ESP_GET_WIFI_LIST,
    ESP_CONNECT_WIFI
} ESP8266_SERVER_HANDLER;

typedef enum ESP8266_TCP_STATUSES
{
    ESP_TCP_CLOSED,
    ESP_TCP_OPENED,
    ESP_TCP_READED
} ESP8266_TCP_STATUS;

typedef enum ESP8266_NOTIFICATIONS
{
    ESP_COMMAND_OK,
    ESP_SEND_OK
} ESP8266_NOTIFICATION;

struct ESP8266
{
    uint8_t initialized;
    char *sta_ssid, *sta_pass;
    size_t sta_ssid_size, sta_pass_size;
    char *ap_ssid, *ap_pass;
    uint8_t ap_chl;
    ESP8266_AP_ENC ap_enc;
};

struct ESP8266_TCP_PACKET
{
    int status, id;
    size_t length;
    char data[ESP_MAX_TCP_SIZE];
};

void esp_server_handler(ESP8266_SERVER_HANDLER handler);

void esp_rx_task(void * const arg);
void wifi_task(void * const arg);
void ESP_InitPins(void);
HAL_StatusTypeDef ESP_InitUART(void);
HAL_StatusTypeDef ESP_InitDMA(void);
void ESP_UART_IRQHandler(UART_HandleTypeDef *huart);

#endif /* ESP8266_WIFI_H */