#include <memory.h>
#include <stdio.h>
#include <limits.h>
#include "math.h"
#include "esp8266_wifi.h"
#include "http_helper.h"
#include "picohttpparser.h"
#include "main.h"

static const TickType_t xBlockTime = pdMS_TO_TICKS(500); // Min time to wait task notification

static struct ESP8266 esp_module = { 0 }; // ESP8266 init struct
static int configure_mode = 1;

UART_HandleTypeDef esp_uart;
DMA_HandleTypeDef esp_dma_rx;
DMA_HandleTypeDef esp_dma_tx;

static uint8_t uart_buffer[ESP_UART_BUFFER_SIZE];
static size_t tmp_size = 0;
static uint8_t tmp_buffer[ESP_UART_BUFFER_SIZE];
static char tcp_buffer[ESP_MAX_TCP_SIZE];
static struct ESP8266_TCP_PACKET tcp_packet = { 0 };

static struct phr_header http_headers[HTTP_MAX_HEADERS];
static struct HTTP_REQUEST http_request = { 0 };
static struct HTTP_RESPONSE http_response = { 0 };

static void notify_wifi_task(uint32_t value);

static void tcp_packet_clear(void);

static size_t get_uart_data_length();
static HAL_StatusTypeDef reset_dma_rx();
static uint32_t wait_uart_rx(void);

static void esp_sysmsg_handler(void);
static void esp_command_handler(uint32_t status);
static void esp_tcp_handler(void);

static void esp_server_answer(void);
static int esp_tcp_send(uint8_t id, size_t size, char *data);

static int esp_connect_wifi(void);
static int esp_drop_wifi(void);

static int esp_send_data(uint8_t *data, size_t data_size);
static int esp_start(void);

void esp_rx_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_ESP_TX_TSK_STARTED);

    for (;;)
    {
        esp_sysmsg_handler();
    }
}

void wifi_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);
    uint32_t status = ESP_TCP_WAIT;

    for (;;)
    {
        if (!esp_module.initialized) esp_module.initialized = esp_start();
        
        xTaskNotifyWait(0x00, ULONG_MAX, &status, portMAX_DELAY);
        if (status == ESP_TCP_READY)
        {
            http_build_response(tcp_buffer, &http_response);
            esp_server_answer();
            http_request_clear(&http_request);
            http_response_clear(&http_response);
            tcp_packet_clear();
        }
        else if (status == ESP_CONF_MODE_ENABLED)
        {
            esp_drop_wifi();
            configure_mode = 1;
        }
        else if (status == ESP_CONF_MODE_DISABLED)
        {
            configure_mode = 0;
        }
    }
}

static void notify_wifi_task(uint32_t value)
{
    configASSERT(wifi_tsk_handle != NULL);
    xTaskNotify(wifi_tsk_handle, value, eSetValueWithOverwrite);
}

static size_t get_uart_data_length(void)
{
    return ESP_UART_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&esp_dma_rx);
}

static HAL_StatusTypeDef reset_dma_rx(void)
{
    if (HAL_UART_DMAStop(&esp_uart) == HAL_ERROR) return HAL_ERROR;
    if (HAL_UART_Receive_DMA(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE) == HAL_ERROR) return HAL_ERROR;

    return HAL_OK;
}

static uint32_t wait_uart_rx(void)
{
    static uint32_t uart_notification = 0;
    uart_notification = ulTaskNotifyTake(pdFALSE, xBlockTime);
    
    return uart_notification;
}

static void tcp_packet_clear(void)
{
    tcp_packet.data = NULL;
    tcp_packet.id = -1;
    tcp_packet.length = 0;
}

static void esp_sysmsg_handler(void)
{
    char *flag;
    if (reset_dma_rx() == HAL_ERROR) return;
    if (!wait_uart_rx()) return;
    
    if ((flag = strstr((char *)uart_buffer, "\r\nOK\r\n")) != NULL)
    { // ESP8266 AT command success message
        tmp_size = get_uart_data_length();
        memcpy(tmp_buffer, uart_buffer, tmp_size);
        esp_command_handler(ESP_COMMAND_OK);
    }
    if ((flag = strstr((char *)uart_buffer, "\r\nFAIL\r\n")) != NULL)
    { // ESP8266 AT command error message
        esp_command_handler(ESP_COMMAND_ERROR);
    }
    if ((flag = strstr((char *)uart_buffer, "\r\nERROR\r\n")) != NULL)
    { // ESP8266 AT command fail message
        esp_command_handler(ESP_COMMAND_ERROR);
    }
    if ((flag = strstr((char *)uart_buffer, "\r\nSEND OK\r\n")) != NULL)
    { // ESP8266 tcp data send success value
        esp_command_handler(ESP_COMMAND_OK);
    }
    if ((flag = strstr((char *)uart_buffer, "ready\r\n")) != NULL)
    { // ESP8266 message on start
        esp_module.initialized = 0;
    }
    if ((flag = strstr((char *)uart_buffer, "+IPD")) != NULL)
    { // ESP8266 message on incoming tcp data
        if (sscanf(flag, "+IPD,%d,%d:", &tcp_packet.id, &tcp_packet.length) < 2) tcp_packet_clear();
        else
        {
            memcpy(tcp_buffer, flag + 8 + NUMBER_LENGTH(tcp_packet.length), tcp_packet.length);
            tcp_packet.data = tcp_buffer;

            esp_tcp_handler();
        }
    }
    if ((flag = strstr((char *)uart_buffer, "CONNECT")) != NULL)
    { // ESP8266 message when tcp client connected

    }
    if ((flag = strstr((char *)uart_buffer, "CLOSED")) != NULL)
    { // ESP8266 message when tcp client closed

    }
}

static void esp_command_handler(uint32_t status)
{
    notify_wifi_task(status);
}

static void esp_tcp_handler(void)
{
    if (tcp_packet.length > 0)
    {
        http_response.version = 1;
        http_request.headers_count = HTTP_MAX_HEADERS;
        int http_res = phr_parse_request(
            (char *)tcp_packet.data, tcp_packet.length, 
            &http_request.method, &http_request.method_size, 
            &http_request.route, &http_request.route_size, &http_request.version, 
            http_headers, &http_request.headers_count
        );
        if (http_res < 0)
        {
            http_response.http_content_type = HTTP_HTML;
            http_response.http_status = HTTP_500;
        }
        else
        {
            if ((http_request.body = strstr(tcp_packet.data, "\r\n\r\n")) != NULL)
            {
                http_request.body += 4;
                http_request.body_size = tcp_packet.length - ((intptr_t)http_request.body - (intptr_t)tcp_packet.data);
            }
            http_check_content_type(&http_response, http_headers, http_request.headers_count);
            if (http_response.http_content_type != HTTP_NOT_ALLOWED)
            {
                http_check_method(&http_response, http_request.method, http_request.method_size);
                if (http_response.http_method != HTTP_NOT_ALLOWED)
                {
                    http_check_route(&http_response, http_request.route, http_request.route_size, configure_mode);
                    if (http_response.route_index < 0) http_response.http_status = HTTP_404;
                    else if (!http_response.availible) http_response.http_status = HTTP_401;
                    else http_response.http_status = HTTP_200;
                }
                else http_response.http_status = HTTP_405;
            }
            else
            {
                http_response.http_content_type = HTTP_HTML;
                http_response.http_status = HTTP_415;
            }
        }
        notify_wifi_task(ESP_TCP_READY);
    }
}

static void esp_server_answer(void)
{
    if (http_response.head_size > 0)
    {
        tcp_packet.length = http_response.head_size;
        tcp_packet.data = tcp_buffer;

        if (esp_tcp_send(tcp_packet.id, tcp_packet.length, tcp_packet.data))
        {
            if (http_response.message_size > 0)
            {
                tcp_packet.length = http_response.message_size;
                if (http_response.message == NULL)
                    tcp_packet.data = tcp_buffer + http_response.head_size;
                else
                    tcp_packet.data = http_response.message;

                esp_tcp_send(tcp_packet.id, tcp_packet.length, tcp_packet.data);
            }
        }
    }
}

void esp_server_handler(ESP8266_SERVER_HANDLER handler)
{
    switch (handler)
    {
    case ESP_VOID_HANDLER:
        
        break;
    case ESP_GET_WIFI_LIST:
        // Get list of wifi networks
        memcpy(uart_buffer, "AT+CWLAP\r\n", 10);
        if (esp_send_data(uart_buffer, 10) == ESP_COMMAND_OK)
        {
            size_t answer_pos = ESP_MAX_TCP_SIZE - tmp_size - 4;
            memcpy(tcp_buffer+answer_pos, tmp_buffer, tmp_size - 4);
            http_response.message = tcp_buffer+answer_pos;
            http_response.message_size = tmp_size - 4;
        }
        else {
            http_response.message = "ERROR";
            http_response.message_size = 5;
        }
        break;
    case ESP_CONNECT_WIFI:
        if (http_request.body_size > 0)
        {
            http_get_form_field(&esp_module.sta_ssid, &esp_module.sta_ssid_size, "ssid=", http_request.body, http_request.body_size);
            http_get_form_field(&esp_module.sta_pass, &esp_module.sta_pass_size, "pass=", http_request.body, http_request.body_size);
            if (!esp_connect_wifi())
            {
                http_response.message = "ERROR";
                http_response.message_size = 5;
            }
            else
            {
                http_response.message = "OK";
                http_response.message_size = 2;
                configure_mode = 0;
            }
        }
        else
        {
            http_response.message = "ERROR";
            http_response.message_size = 5;
        }
        break;
    default:
        break;
    }
}

static int esp_connect_wifi(void)
{
    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=1\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_COMMAND_ERROR) return 0;

    sprintf((char *)uart_buffer, "AT+CWJAP_DEF=\"%.*s\",\"%.*s\"\r\n", (int)esp_module.sta_ssid_size, esp_module.sta_ssid, (int)esp_module.sta_pass_size, esp_module.sta_pass);
    if (esp_send_data(uart_buffer, 20 + esp_module.sta_ssid_size + esp_module.sta_pass_size) == ESP_COMMAND_ERROR) return 0;

    return 1;
}

static int esp_drop_wifi(void)
{
    memcpy(uart_buffer, (uint8_t *)"AT+CWQAP\r\n", 10);
    if (esp_send_data(uart_buffer, 10) == ESP_COMMAND_ERROR) return 0;

    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=0\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_COMMAND_ERROR) return 0;

    return 1;
}

static int esp_tcp_send(uint8_t id, size_t size, char *data)
{
    if (size <= ESP_MAX_TCP_SIZE)
    {
        sprintf((char *)uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, size);
        if (esp_send_data(uart_buffer, 15 + NUMBER_LENGTH(size)) == ESP_COMMAND_ERROR) return -1;
        if (esp_send_data((uint8_t *)data, size) == ESP_COMMAND_ERROR) return -1;
    }
    else
    {
        size_t packets_count = size / ESP_MAX_TCP_SIZE;
        for (uint16_t i = 0; i < packets_count; i++)
        {
            sprintf((char *)uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, ESP_MAX_TCP_SIZE);
            if (esp_send_data(uart_buffer, 15 + NUMBER_LENGTH(ESP_MAX_TCP_SIZE)) == ESP_COMMAND_ERROR) return -1;
            if (esp_send_data((uint8_t *)(data+(i*ESP_MAX_TCP_SIZE)), ESP_MAX_TCP_SIZE) == ESP_COMMAND_ERROR) return -1;
        }
        if ((packets_count * ESP_MAX_TCP_SIZE) < size)
        {
            size_t tail = size - (packets_count * ESP_MAX_TCP_SIZE);
            sprintf((char *)uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, tail);
            if (esp_send_data(uart_buffer, 15 + NUMBER_LENGTH(tail)) == ESP_COMMAND_ERROR) return -1;
            if (esp_send_data((uint8_t *)(data+(packets_count * ESP_MAX_TCP_SIZE)), tail) == ESP_COMMAND_ERROR) return -1;
        }
    }
    
    return size;
}

static int esp_send_data(uint8_t *data, size_t data_size)
{
    HAL_UART_Transmit(&esp_uart, data, data_size, ESP_UART_DELAY);
    uint32_t status = ESP_COMMAND_ERROR;
    xTaskNotifyWait(0x00, ULONG_MAX, &status, portMAX_DELAY);

    return status;
}

static int esp_start(void)
{
    esp_module.ap_ssid = "AirC Device";
    esp_module.ap_pass = "314159265";
    esp_module.ap_chl = 3;
    esp_module.ap_enc = WPA2_PSK;

    // Disable AT commands echo
    memcpy(uart_buffer, (uint8_t *)"ATE0\r\n", 6);
    if (esp_send_data(uart_buffer, 6) == ESP_COMMAND_ERROR) return 0;

    // Set soft AP + station mode
    memcpy(uart_buffer, (uint8_t *)"AT+CWMODE_DEF=3\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_COMMAND_ERROR) return 0;

    // Set auto connect to saved wifi network
    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=1\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_COMMAND_ERROR) return 0;

    // Set IP for soft AP
    memcpy(uart_buffer, (uint8_t *)"AT+CIPAP_DEF=\"192.168.4.1\"\r\n", 28);
    if (esp_send_data(uart_buffer, 28) == ESP_COMMAND_ERROR) return 0;

    // Configure soft AP
    sprintf((char *)uart_buffer, "AT+CWSAP_DEF=\"%s\",\"%s\",%d,%d\r\n", 
        esp_module.ap_ssid, esp_module.ap_pass, 
        esp_module.ap_chl, esp_module.ap_enc);
    if (esp_send_data(uart_buffer, 24 + strlen(esp_module.ap_ssid) + strlen(esp_module.ap_pass)) == ESP_COMMAND_ERROR) return 0;

    // Allow multiple TCP connections
    memcpy(uart_buffer, "AT+CIPMUX=1\r\n", 13);
    if (esp_send_data(uart_buffer, 13) == ESP_COMMAND_ERROR) return 0;

    // Start server
    sprintf((char *)uart_buffer, "AT+CIPSERVER=1,%d\r\n", HTTP_SERVER_PORT);
    if (esp_send_data(uart_buffer, 17 + NUMBER_LENGTH(HTTP_SERVER_PORT)) == ESP_COMMAND_ERROR) return 0;

    /* Temporary block of code need while button isn't connected */
    esp_drop_wifi();
    /* --- */

    return 1;
}

void ESP_InitPins(void)
{
    /* PINS: TX - PC6, RX - PC7*/
    GPIO_InitTypeDef gpio;
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
    HAL_GPIO_Init(GPIOC, &gpio);
}

HAL_StatusTypeDef ESP_InitUART(void)
{
    /* USART 6 */
    esp_uart.Instance = USART6;
    esp_uart.Init.BaudRate = 115200;
    esp_uart.Init.WordLength = UART_WORDLENGTH_8B;
    esp_uart.Init.StopBits = UART_STOPBITS_1;
    esp_uart.Init.Parity = UART_PARITY_NONE;
    esp_uart.Init.Mode = UART_MODE_TX_RX;
    esp_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    esp_uart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&esp_uart) == HAL_ERROR) return HAL_ERROR;

    HAL_NVIC_SetPriority(USART6_IRQn, ESP_INT_PRIO, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    __HAL_UART_ENABLE_IT(&esp_uart, UART_IT_IDLE);

    return HAL_OK;
}

HAL_StatusTypeDef ESP_InitDMA(void)
{
    esp_dma_tx.Instance = DMA2_Stream6;
    esp_dma_tx.Init.Channel = DMA_CHANNEL_5;
    esp_dma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    esp_dma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    esp_dma_tx.Init.MemInc = DMA_MINC_ENABLE;
    esp_dma_tx.Init.Mode = DMA_NORMAL;
    esp_dma_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    esp_dma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    esp_dma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    esp_dma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&esp_dma_tx) == HAL_ERROR) return HAL_ERROR;
    __HAL_LINKDMA(&esp_uart, hdmatx, esp_dma_tx);

    esp_dma_rx.Instance = DMA2_Stream1;
    esp_dma_rx.Init.Channel = DMA_CHANNEL_5;
    esp_dma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    esp_dma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    esp_dma_rx.Init.MemInc = DMA_MINC_ENABLE;
    esp_dma_rx.Init.Mode = DMA_NORMAL;
    esp_dma_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    esp_dma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    esp_dma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    esp_dma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&esp_dma_rx) == HAL_ERROR) return HAL_ERROR;
    __HAL_LINKDMA(&esp_uart, hdmarx, esp_dma_rx);

    return HAL_OK;
}

void ESP_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    if (USART6 == huart->Instance)
    {
        configASSERT(esp_rx_tsk_handle != NULL);

        if(RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
        {
            __HAL_UART_CLEAR_IDLEFLAG(huart);

            BaseType_t task_woken = pdFALSE;
            vTaskNotifyGiveFromISR(esp_rx_tsk_handle, &task_woken);
            portYIELD_FROM_ISR(task_woken);
        }
    }     
}