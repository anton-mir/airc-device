#include <memory.h>
#include <stdio.h>
#include <limits.h>
#include "math.h"
#include "esp8266_wifi.h"
#include "http_helper.h"
#include "number_helper.h"
#include "config_board.h"
#include "picohttpparser.h"
#include "main.h"

extern boxConfig_S device_config;

static const TickType_t xWifiBlockTime = pdMS_TO_TICKS(20000);

static char binary_buf;
static char number_buf[ULL_STRING_LENGTH];

static struct ESP8266 esp_module = { 0 }; // ESP8266 init struct
volatile int esp_server_mode = 0;

UART_HandleTypeDef esp_uart = { 0 };
DMA_HandleTypeDef esp_dma_rx = { 0 };

static size_t uart_data_size = 0;
static uint8_t uart_buffer[ESP_UART_BUFFER_SIZE];

static struct ESP8266_TCP_PACKET tcp_packet = { 0 };
static char tcp_buffer[ESP_MAX_TCP_SIZE];
static char http_buffer[ESP_MAX_TCP_SIZE];
static char networks_list_buffer[ESP_UART_BUFFER_SIZE];

static struct phr_header http_headers[HTTP_MAX_HEADERS];
static struct HTTP_REQUEST http_request = { 0 };
static struct HTTP_RESPONSE http_response = { 0 };
static struct HTTP_FORM_VALUE http_form_value = { 0 };

static int esp_tcp_send(uint8_t id, size_t size, char *data);
static uint32_t esp_send_data(uint8_t *data, size_t data_size);
static uint32_t esp_start(void);
static uint32_t esp_connect_wifi();
static int check_ascii(char *str, size_t str_size);

void esp_rx_task(void * const arg)
{
    char *pos;

    for (;;)
    {
        HAL_UART_DMAStop(&esp_uart);
        HAL_UART_Receive_DMA(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uart_data_size = ESP_UART_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&esp_dma_rx);

        if (strstr((char *)uart_buffer, "\r\nOK\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_OK, eSetValueWithOverwrite);
        }
        else if (strstr((char *)uart_buffer, "\r\nERROR\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_ERROR, eSetValueWithOverwrite);
        }
        else if (strstr((char *)uart_buffer, "\r\nFAIL\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_ERROR, eSetValueWithOverwrite);
        }
        else if (strstr((char *)uart_buffer, "\r\nSEND OK\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_OK, eSetValueWithOverwrite);
        }
        else if (strstr((char *)uart_buffer, "\r\nSEND FAIL\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_ERROR, eSetValueWithOverwrite);
        }
        else if ((pos = strstr((char *)uart_buffer, "+IPD,")) != NULL)
        {
            if (tcp_packet.status == ESP_TCP_CLOSED)
            {
                int res = sscanf(pos, "+IPD,%d,%d:", &tcp_packet.id, &tcp_packet.length);
                if (res == 2)
                {
                    memcpy(tcp_buffer, pos + 8 + NUMBER_LENGTH(tcp_packet.length), tcp_packet.length);
                    tcp_packet.status = ESP_TCP_OPENED;
                }
            }
        }
    }
}

void wifi_task(void * const arg)
{
    esp_start();

    for (;;)
    {
        if (tcp_packet.status == ESP_TCP_OPENED && tcp_packet.length > 0)
        {
            http_request.headers_count = HTTP_MAX_HEADERS;
            int http_res = phr_parse_request(
                tcp_buffer, tcp_packet.length,
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
                if ((http_request.body = strstr(tcp_buffer, "\r\n\r\n")) != NULL)
                {
                    http_request.body += 4;
                    http_request.body_size = tcp_packet.length - ((intptr_t)http_request.body - (intptr_t)tcp_buffer);
                }
                http_check_content_type(&http_response, http_headers, http_request.headers_count);
                if (http_response.http_content_type != HTTP_NOT_ALLOWED)
                {
                    http_check_method(&http_response, http_request.method, http_request.method_size);
                    if (http_response.http_method != HTTP_NOT_ALLOWED)
                    {
                        http_check_route(http_headers, http_request.headers_count, &http_response, http_request.route, http_request.route_size, esp_server_mode);
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

            http_build_response(tcp_buffer, &http_response);

            if (http_response.head_size > 0)
            {
                tcp_packet.length = http_response.head_size;

                if (esp_tcp_send(tcp_packet.id, tcp_packet.length, tcp_buffer) >= 0)
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
            sprintf((char *)uart_buffer, "AT+CIPCLOSE=%d\r\n", tcp_packet.id);
            esp_send_data(uart_buffer, 15);
            tcp_packet.status = ESP_TCP_CLOSED;
            memset(tcp_buffer, 0, ESP_MAX_TCP_SIZE);
        }
        vTaskDelay(500);
    }
}

void esp_server_handler(ESP8266_SERVER_HANDLER handler)
{
    char latitude_buffer[10], *latitude_str;
    char longitude_buffer[11], *longitude_str;
    char altitude_buffer[7], *altitude_str;
    char so2_id_buffer[ULL_STRING_LENGTH], *so2_id_str;
    char no2_id_buffer[ULL_STRING_LENGTH], *no2_id_str;
    char co_id_buffer[ULL_STRING_LENGTH], *co_id_str;
    char o3_id_buffer[ULL_STRING_LENGTH], *o3_id_str;

		size_t response_length = 0, length = 0;

    switch (handler)
    {
    case ESP_GET_WIFI_LIST:
		http_response.message = networks_list_buffer;
        http_response.message_size = strlen(http_response.message);
        break;
    case ESP_GET_DEVICE_CONF:
        ReadConfig(&device_config);
        latitude_str = ftoa(device_config.latitude, latitude_buffer, 6);
        longitude_str = ftoa(device_config.longitude, longitude_buffer, 6);
        altitude_str = ftoa(device_config.altitude, altitude_buffer, 2);
        so2_id_str = ulltoa(device_config.SO2_specSN, so2_id_buffer);
        no2_id_str = ulltoa(device_config.NO2_specSN, no2_id_buffer);
        co_id_str = ulltoa(device_config.CO_specSN, co_id_buffer);
        o3_id_str = ulltoa(device_config.O3_specSN, o3_id_buffer);
        http_response.message_size = sprintf(http_buffer,
        "{\"id\":%d,\"ip\":\"%s\",\"fb_email\":\"%s\",\"fb_pass\":\"%s\",\"type\":\"%s\",\"desc\":\"%s\",\"lat\":%s,\"long\":%s,\"alt\":%s,\"mode\":%d,\"so2_id\":%s,\"no2_id\":%s,\"co_id\":%s,\"o3_id\":%s}",
        device_config.id, device_config.ip, device_config.fb_email, device_config.fb_pass, device_config.type, device_config.description,
        latitude_str, longitude_str, altitude_str, device_config.working_status,
        so2_id_str, no2_id_str, co_id_str, o3_id_str);
        http_response.message = http_buffer;
        break;
    case ESP_SET_DEVICE_CONF:
        if (http_request.body_size > 0)
        {
            http_get_form_field(&http_form_value.value, &http_form_value.size, "id=", http_request.body, http_request.body_size);
            memcpy(number_buf, http_form_value.value, http_form_value.size);
            device_config.id = atoi(number_buf);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "mode=", http_request.body, http_request.body_size);
            memcpy(&binary_buf, http_form_value.value, 1);
            device_config.working_status = binary_buf - '0';

            http_get_form_field(&http_form_value.value, &http_form_value.size, "type=", http_request.body, http_request.body_size);
            memset(device_config.type, 0, sizeof(device_config.type));
            memcpy(device_config.type, http_form_value.value, http_form_value.size);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "ip=", http_request.body, http_request.body_size);
            memset(device_config.ip, 0, sizeof(device_config.ip));
            memcpy(device_config.ip, http_form_value.value, http_form_value.size);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "fb_email=", http_request.body, http_request.body_size);
            memset(device_config.fb_email, 0, sizeof(device_config.fb_email));
            memcpy(device_config.fb_email, http_form_value.value, http_form_value.size);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "fb_pass=", http_request.body, http_request.body_size);
            memset(device_config.fb_pass, 0, sizeof(device_config.fb_pass));
            memcpy(device_config.fb_pass, http_form_value.value, http_form_value.size);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "desc=", http_request.body, http_request.body_size);
            memset(device_config.description, 0, sizeof(device_config.description));
            memcpy(device_config.description, http_form_value.value, http_form_value.size);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "lat=", http_request.body, http_request.body_size);
            memset(latitude_buffer, 0, 10);
            memcpy(latitude_buffer, http_form_value.value, http_form_value.size);
            device_config.latitude = atof(latitude_buffer);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "long=", http_request.body, http_request.body_size);
            memset(longitude_buffer, 0, 11);
            memcpy(longitude_buffer, http_form_value.value, http_form_value.size);
            device_config.longitude = atof(longitude_buffer);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "alt=", http_request.body, http_request.body_size);
            memset(altitude_buffer, 0, 7);
            memcpy(altitude_buffer, http_form_value.value, http_form_value.size);
            device_config.altitude = atof(altitude_buffer);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "so2_id=", http_request.body, http_request.body_size);
            memset(so2_id_buffer, 0, ULL_STRING_LENGTH);
            memcpy(so2_id_buffer, http_form_value.value, http_form_value.size);
            device_config.SO2_specSN = atoull(so2_id_buffer);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "no2_id=", http_request.body, http_request.body_size);
            memset(no2_id_buffer, 0, ULL_STRING_LENGTH);
            memcpy(no2_id_buffer, http_form_value.value, http_form_value.size);
            device_config.NO2_specSN = atoull(no2_id_buffer);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "co_id=", http_request.body, http_request.body_size);
            memset(co_id_buffer, 0, ULL_STRING_LENGTH);
            memcpy(co_id_buffer, http_form_value.value, http_form_value.size);
            device_config.CO_specSN = atoull(co_id_buffer);

            http_get_form_field(&http_form_value.value, &http_form_value.size, "o3_id=", http_request.body, http_request.body_size);
            memset(o3_id_buffer, 0, ULL_STRING_LENGTH);
            memcpy(o3_id_buffer, http_form_value.value, http_form_value.size);
            device_config.O3_specSN = atoull(o3_id_buffer);

            WriteConfig(device_config);

            http_response.message = "OK";
            http_response.message_size = 2;
        }
        else
        {
            http_response.message = "ERROR";
            http_response.message_size = 5;
        }
        break;
    case ESP_CONNECT_WIFI:
        if (http_request.body_size > 0)
        {
            http_get_form_field(&esp_module.sta_ssid, &esp_module.sta_ssid_size, "ssid=", http_request.body, http_request.body_size);
            http_get_form_field(&esp_module.sta_pass, &esp_module.sta_pass_size, "pass=", http_request.body, http_request.body_size);

            if (esp_connect_wifi() == ESP_OK)
            {
                memset(device_config.wifi_ssid, 0, sizeof(device_config.wifi_ssid));
                memcpy(device_config.wifi_ssid, esp_module.sta_ssid, esp_module.sta_ssid_size);
                esp_module.sta_ssid = device_config.wifi_ssid;
                memset(device_config.wifi_pass, 0, sizeof(device_config.wifi_pass));
                memcpy(device_config.wifi_pass, esp_module.sta_pass, esp_module.sta_pass_size);
                esp_module.sta_pass = device_config.wifi_pass;

                WriteConfig(device_config);

                http_response.message = "OK";
                http_response.message_size = 2;
            }
            else
            {
                esp_module.sta_ssid = device_config.wifi_ssid;
                esp_module.sta_pass = device_config.wifi_pass;
                http_response.message = "ERROR";
                http_response.message_size = 5;
            }
        }
        else
        {
            http_response.message = "ERROR";
            http_response.message_size = 5;
        }
        break;
    case ESP_WIFI_MODE:
        if (http_request.body_size > 0)
        {
            char *mode;
            size_t mode_size;
            http_get_form_field(&mode, &mode_size, "mode=", http_request.body, http_request.body_size);

            if (memcmp(mode, "on", mode_size) == 0)
            {
                if (esp_connect_wifi())
                {
                    http_response.message = "OK";
                    http_response.message_size = 2;
                }
                else
                {
                    http_response.message = "ERROR";
                    http_response.message_size = 5;
                }

            }
            else if (memcmp(mode, "off", mode_size) == 0)
            {
                memcpy(uart_buffer, (uint8_t *)"AT+CWQAP\r\n", 10);
                if (esp_send_data(uart_buffer, 10) == ESP_OK)
                {
                    http_response.message = "OK";
                    http_response.message_size = 2;
                }
                else
                {
                    http_response.message = "ERROR";
                    http_response.message_size = 5;
                }
            }
            else
            {
                http_response.message = "ERROR";
                http_response.message_size = 5;
            }
        }
        else
        {
            http_response.message = "ERROR";
            http_response.message_size = 5;
        }
        break;
    case ESP_CONF_MODE:
        if (http_request.body_size > 0)
        {
            char *mode;
            size_t mode_size;
            http_get_form_field(&mode, &mode_size, "mode=", http_request.body, http_request.body_size);

            if (memcmp(mode, "on", mode_size) == 0)
            {
                esp_server_mode = 1;
                http_response.message = "OK";
                http_response.message_size = 2;
            }
            else if (memcmp(mode, "off", mode_size) == 0)
            {
                esp_server_mode = 0;
                http_response.message = "OK";
                http_response.message_size = 2;
            }
            else
            {
                http_response.message = "ERROR";
                http_response.message_size = 5;
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

static int esp_tcp_send(uint8_t id, size_t size, char *data)
{
    if (size <= ESP_MAX_TCP_SIZE)
    {
        sprintf((char *)uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, size);
        if (esp_send_data(uart_buffer, 15 + NUMBER_LENGTH(size)) == ESP_OK);
        else return -1;

        if (esp_send_data((uint8_t *)data, size) == ESP_OK);
        else return -1;
    }
    else
    {
        size_t packets_count = size / ESP_MAX_TCP_SIZE;
        for (uint16_t i = 0; i < packets_count; i++)
        {
            sprintf((char *)uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, ESP_MAX_TCP_SIZE);
            if (esp_send_data(uart_buffer, 15 + NUMBER_LENGTH(ESP_MAX_TCP_SIZE)) == ESP_OK);
            else return -1;

            if (esp_send_data((uint8_t *)(data+(i*ESP_MAX_TCP_SIZE)), ESP_MAX_TCP_SIZE) == ESP_OK);
            else return -1;
        }
        if ((packets_count * ESP_MAX_TCP_SIZE) < size)
        {
            size_t tail = size - (packets_count * ESP_MAX_TCP_SIZE);
            sprintf((char *)uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, tail);
            if (esp_send_data(uart_buffer, 15 + NUMBER_LENGTH(tail)) == ESP_OK);
            else return -1;

            if (esp_send_data((uint8_t *)(data+(packets_count * ESP_MAX_TCP_SIZE)), tail) == ESP_OK);
            else return -1;
        }
    }

    return size;
}

static uint32_t esp_send_data(uint8_t *data, size_t data_size)
{
    static uint32_t status = ESP_ERROR;

    HAL_UART_Transmit(&esp_uart, data, data_size, ESP_UART_DELAY);
    memset(uart_buffer, 0, ESP_UART_BUFFER_SIZE);
    status = ulTaskNotifyTake(pdTRUE, xWifiBlockTime);

    return status;
}

static uint32_t esp_start(void)
{
    ReadConfig(&device_config);

    esp_module.ap_ssid = "AirC Device";
    esp_module.ap_pass = "314159265";
    esp_module.ap_chl = 3;
    esp_module.ap_enc = WPA2_PSK;
    esp_module.sta_ssid = device_config.wifi_ssid;
    esp_module.sta_ssid_size = strlen(device_config.wifi_ssid);
    esp_module.sta_pass = device_config.wifi_pass;
    esp_module.sta_pass_size = strlen(device_config.wifi_pass);

    // Disable AT commands echo
    memcpy(uart_buffer, (uint8_t *)"ATE0\r\n", 6);
    if (esp_send_data(uart_buffer, 6) == ESP_OK);
    else return 0;

    // Set soft AP + station mode
    memcpy(uart_buffer, (uint8_t *)"AT+CWMODE_DEF=3\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_OK);
    else return 0;

    memcpy(uart_buffer, (uint8_t *)"AT+CWLAP\r\n", 10);
	if (esp_send_data(uart_buffer, 10) == ESP_OK)
    {
		memcpy(networks_list_buffer, uart_buffer, uart_data_size - 6);
    }
    else
    {
        memcpy(networks_list_buffer, "ERROR", 5);
    }

	// Set auto connect to saved wifi network
    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=0\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_OK);
    else return 0;

    // Set IP for soft AP
    memcpy(uart_buffer, (uint8_t *)("AT+CIPAP_DEF=\"" ESP_SERVER_HOST "\"\r\n"), 17 + strlen(ESP_SERVER_HOST));
    if (esp_send_data(uart_buffer, 17 + strlen(ESP_SERVER_HOST)) == ESP_OK);
    else return 0;

    // Configure soft AP
    sprintf((char *)uart_buffer, "AT+CWSAP_DEF=\"%s\",\"%s\",%d,%d\r\n",
        esp_module.ap_ssid, esp_module.ap_pass,
        esp_module.ap_chl, esp_module.ap_enc);
    if (esp_send_data(uart_buffer, 23 + strlen(esp_module.ap_ssid) + strlen(esp_module.ap_pass) + NUMBER_LENGTH(esp_module.ap_chl)) == ESP_OK);
    else return 0;

    // Allow multiple TCP connections
    memcpy(uart_buffer, "AT+CIPMUX=1\r\n", 13);
    if (esp_send_data(uart_buffer, 13) == ESP_OK);
    else return 0;

    // Start server
    sprintf((char *)uart_buffer, "AT+CIPSERVER=1,%d\r\n", HTTP_SERVER_PORT);
    if (esp_send_data(uart_buffer, 17 + NUMBER_LENGTH(HTTP_SERVER_PORT)) == ESP_OK);
    else return 0;

    esp_connect_wifi();

    return 1;
}

static uint32_t esp_connect_wifi()
{
    if (!check_ascii(esp_module.sta_ssid, esp_module.sta_ssid_size)) return 0;
    if (!check_ascii(esp_module.sta_pass, esp_module.sta_pass_size)) return 0;
    sprintf((char *)uart_buffer, "AT+CWJAP_DEF=\"%.*s\",\"%.*s\"\r\n", (int)esp_module.sta_ssid_size, esp_module.sta_ssid, (int)esp_module.sta_pass_size, esp_module.sta_pass);
    if (esp_send_data(uart_buffer, 20 + esp_module.sta_ssid_size + esp_module.sta_pass_size) == ESP_OK) return 1;
    else return 0;
}

static int check_ascii(char *str, size_t str_size)
{
    if (str_size == 0) return 1;
    for (size_t i = 0; i < str_size; i++)
    {
        if (str[i] > 0x7F)
            return 0;
    }
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
        if(RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
        {
            __HAL_UART_CLEAR_IDLEFLAG(huart);

            configASSERT(esp_rx_tsk_handle != NULL);

            BaseType_t task_woken = pdTRUE;
            vTaskNotifyGiveFromISR(esp_rx_tsk_handle, &task_woken);
            portYIELD_FROM_ISR(task_woken);
        }
    }
}
