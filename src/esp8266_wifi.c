#include "esp8266_wifi.h"
#include "http_helper.h"
#include "picohttpparser.h"
#include "main.h"

const TickType_t xMaxBlockTime = pdMS_TO_TICKS(20000); // Max time to wait task notification
const TickType_t xMinBlockTime = pdMS_TO_TICKS(200); // Min time to wait task notification

const uint8_t *AT_OK = (uint8_t *)"\r\nOK\r\n"; // ESP8266 AT command success value
const uint8_t *SEND_OK = (uint8_t *)"\r\nSEND OK\r\n"; // ESP8266 tcp data send success value
const uint8_t *STA_GOT_IP = (uint8_t *)"STATUS:2\r\n"; // ESP8266 status when it got IP in network
const uint8_t *STA_CONNECTED = (uint8_t *)"STATUS:3\r\n"; // ESP8266 status when it conected to AP
const uint8_t *STA_DISCONNECTED = (uint8_t *)"STATUS:4\r\n"; // ESP8266 status when it disconnected from AP
const uint8_t *STA_NO_AP = (uint8_t *)"STATUS:5\r\n"; // ESP8266 status when it not connected to AP
const uint8_t *IPD_FLAG = (uint8_t *)"+IPD"; // ESP8266 message for incoming tcp data
const uint8_t *READY_FLAG = (uint8_t *)"ready"; // ESP8266 message on start

struct ESP8266 esp_module = { 0 };

UART_HandleTypeDef esp_uart;
DMA_HandleTypeDef esp_dma_rx;
DMA_HandleTypeDef esp_dma_tx;

static uint8_t uart_buffer[ESP_UART_BUFFER_SIZE];
static uint8_t tcp_buffer[ESP_MAX_TCP_SIZE];

static uint16_t esp_recv_buf[ESP_MAX_TCP_CONNECTIONS];
static struct ESP8266_TCP_PACKET tcp_packet = { 0 };

struct phr_header http_headers[HTTP_MAX_HEADERS];
struct HTTP_REQUEST http_request = { 0 };
struct HTTP_RESPONSE http_response = { 0 };

static size_t get_uart_data_length();
static HAL_StatusTypeDef reset_dma_rx();
static int wait_uart_rx(TickType_t delay);
static uint8_t *check_uart_flag(uint8_t *flag);
static int send_to_esp(uint8_t *data, size_t data_size, const uint8_t *answer, TickType_t delay);
static void esp_sysmsg_handle(void);
static int esp_clear_buf(uint8_t id);
static void server_response(void);
static int esp_send(uint8_t id, size_t size, uint8_t *data);
static int esp_start_server(void);
static int esp_drop_server(void);
static int esp_check_wifi(void);
static int esp_connect_wifi(void);
static int esp_drop_wifi(void);
static int esp_start(void);

void wifi_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);

    for (;;)
    {
        if (!esp_module.initialized)
        {
            // Configure ESP8266
            if (!esp_start()) continue;

            esp_module.initialized = 1;
        }
        else
        {
            esp_sysmsg_handle();
            if (tcp_packet.length > 0)
            {
                http_response.version = 1;

                http_request.headers_count = HTTP_MAX_HEADERS;
                int res = phr_parse_request(
                    (char *)tcp_packet.data, tcp_packet.length, 
                    &http_request.method, &http_request.method_size, 
                    &http_request.route, &http_request.route_size, &http_request.version, 
                    http_headers, &http_request.headers_count
                );
                if (res < 0)
                {
                    http_response.message = "500 Internal Server Error";
                    http_build_text_response(
                        tcp_buffer, http_response.message,
                        &http_response.message_size, &http_response.head_size,
                        http_request.route, http_request.route_size, 500
                    );
                    server_response();
                }
                else
                {
                    if (memcmp(http_request.method, "GET", http_request.method_size) == 0)
                    {
                        http_build_html_response(
                            tcp_buffer, &http_response.message,
                            &http_response.message_size, &http_response.head_size,
                            http_request.route, http_request.route_size
                        );
                        server_response();
                    }
                    else if (memcmp(http_request.method, "POST", http_request.method_size) == 0)
                    {
                        if (memcmp(http_request.route+1, "network", 7) == 0)
                        {
                            if ((http_request.body = strstr(tcp_packet.data, "\r\n\r\n")) != NULL)
                            {
                                http_request.body += 4;
                                http_request.body_size = tcp_packet.length - ((intptr_t)http_request.body - (intptr_t)tcp_packet.data);
                                if (http_request.body_size > 0)
                                {
                                    http_get_form_field(&esp_module.sta_ssid, &esp_module.sta_ssid_size, "ssid=", http_request.body, http_request.body_size);
                                    http_get_form_field(&esp_module.sta_pass, &esp_module.sta_pass_size, "pass=", http_request.body, http_request.body_size);

                                    if (esp_module.sta_ssid != NULL && esp_module.sta_pass != NULL)
                                    {
                                        if (esp_connect_wifi())
                                        {
                                            http_response.message = "OK";
                                            http_build_text_response(
                                                tcp_buffer, http_response.message,
                                                &http_response.message_size, &http_response.head_size,
                                                http_request.route, http_request.route_size, 200
                                            );
                                            server_response();
                                        }
                                        else
                                        {
                                            http_response.message = "ERROR";
                                            http_build_text_response(
                                                tcp_buffer, http_response.message,
                                                &http_response.message_size, &http_response.head_size,
                                                http_request.route, http_request.route_size, 200
                                            );
                                            server_response();
                                        }
                                    }
                                    else
                                    {
                                        http_response.message = "ERROR";
                                        http_build_text_response(
                                            tcp_buffer, http_response.message,
                                            &http_response.message_size, &http_response.head_size,
                                            http_request.route, http_request.route_size, 200
                                        );
                                        server_response();
                                    }
                                }
                                else
                                {
                                    http_response.message = "ERROR";
                                    http_build_text_response(
                                        tcp_buffer, http_response.message,
                                        &http_response.message_size, &http_response.head_size,
                                        http_request.route, http_request.route_size, 200
                                    );
                                    server_response();
                                }
                            }
                            else
                            {
                                http_response.message = "ERROR";
                                http_build_text_response(
                                    tcp_buffer, http_response.message,
                                    &http_response.message_size, &http_response.head_size,
                                    http_request.route, http_request.route_size, 200
                                );
                                server_response();
                            }
                        }
                        else
                        {
                            http_response.message = "404 Not Found";
                            http_build_text_response(
                                tcp_buffer, http_response.message,
                                &http_response.message_size, &http_response.head_size,
                                http_request.route, http_request.route_size, 404
                            );
                            server_response();
                        }
                    }
                    else
                    {
                        http_response.message = "405 Method Not Allowed";
                        http_build_text_response(
                            tcp_buffer, http_response.message,
                            &http_response.message_size, &http_response.head_size,
                            http_request.route, http_request.route_size, 405
                        );
                        server_response();
                    }
                }

                tcp_packet.length = 0;
                tcp_packet.data = NULL;
            }
        }
        vTaskDelay(500);
    }
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

static int wait_uart_rx(TickType_t delay)
{
    static uint32_t uart_notification;
    uart_notification = ulTaskNotifyTake(pdFALSE, delay);

    if (!uart_notification) return 0;

    return 1;
}

static uint8_t *check_uart_flag(uint8_t *flag)
{
    return strstr(uart_buffer, flag);
}

static int send_to_esp(uint8_t *data, size_t data_size, const uint8_t *answer, TickType_t delay)
{
    HAL_UART_Transmit(&esp_uart, data, data_size, ESP_UART_DELAY);
    if (answer == NULL) return 1;

    if (reset_dma_rx() == HAL_ERROR) return 0;
    if (!wait_uart_rx(delay)) return 0;

    if (check_uart_flag(answer) == NULL) return 0;

    return 1;
}

static void esp_sysmsg_handle(void)
{
    uint8_t *message;
    if (reset_dma_rx() == HAL_ERROR) return;
    if (!wait_uart_rx(xMinBlockTime)) return;
    
    if ((message = strstr(uart_buffer, IPD_FLAG)) != NULL)
    {
        sscanf(message, "+IPD,%d,%d:", &tcp_packet.id, &tcp_packet.length);
        memcpy(tcp_buffer, strstr(message, ":") + 1, tcp_packet.length);
        tcp_packet.data = tcp_buffer;
    }
    else if ((message = strstr(uart_buffer, READY_FLAG)) != NULL)
    {
        esp_module.initialized = 0;
    }
}

static void server_response(void)
{
    tcp_packet.length = http_response.head_size;
    tcp_packet.data = tcp_buffer;

    esp_send(tcp_packet.id, tcp_packet.length, tcp_packet.data);

    tcp_packet.length = http_response.message_size;
    if (http_response.message == NULL)
        tcp_packet.data = tcp_buffer + http_response.head_size;
    else
        tcp_packet.data = (uint8_t *)http_response.message;

    esp_send(tcp_packet.id, tcp_packet.length, tcp_packet.data);
}

static int esp_send(uint8_t id, size_t size, uint8_t *data)
{
    if (size <= ESP_MAX_TCP_SIZE)
    {
        sprintf(uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, size);
        if (!send_to_esp(uart_buffer, 15 + NUMBER_LENGTH(size), AT_OK, xMinBlockTime)) return -1;

        send_to_esp(data, size, SEND_OK, xMaxBlockTime);
    }
    else
    {
        size_t packets_count = size / ESP_MAX_TCP_SIZE;
        for (uint16_t i = 0; i < packets_count; i++)
        {
            sprintf(uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, ESP_MAX_TCP_SIZE);
            if (!send_to_esp(uart_buffer, 15 + NUMBER_LENGTH(ESP_MAX_TCP_SIZE), AT_OK, xMinBlockTime)) return -1;

            send_to_esp(data+(i*ESP_MAX_TCP_SIZE), ESP_MAX_TCP_SIZE, SEND_OK, xMaxBlockTime);
        }
        if ((packets_count * ESP_MAX_TCP_SIZE) < size)
        {
            size_t tail = size - (packets_count * ESP_MAX_TCP_SIZE);
            sprintf(uart_buffer, "AT+CIPSEND=%d,%d\r\n", id, tail);
            if (!send_to_esp(uart_buffer, 15 + NUMBER_LENGTH(tail), AT_OK, xMinBlockTime)) return -1;

            send_to_esp(data+(packets_count * ESP_MAX_TCP_SIZE), tail, SEND_OK, xMaxBlockTime);
        }
    }
    
    return size;
}

static int esp_start_server(void)
{
    // Start server
    sprintf(uart_buffer, "AT+CIPSERVER=1,%d\r\n", HTTP_SERVER_PORT);
    if (!send_to_esp(uart_buffer, 17 + NUMBER_LENGTH(HTTP_SERVER_PORT), AT_OK, xMinBlockTime)) return 0;

    // Set server timeout to 10s
    memcpy(uart_buffer, "AT+CIPSTO=20\r\n", 14);
    if (!send_to_esp(uart_buffer, 14, AT_OK, xMinBlockTime)) return 0;

    return 1;
}

static int esp_drop_server(void)
{
    // Stop server
    sprintf(uart_buffer, "AT+CIPSERVER=0,%d\r\n", HTTP_SERVER_PORT);
    if (!send_to_esp(uart_buffer, 17 + NUMBER_LENGTH(HTTP_SERVER_PORT), AT_OK, xMinBlockTime)) return 0;

    // Restart ESP8266
    memcpy(uart_buffer, "AT+RST\r\n", 8);
    if (!send_to_esp(uart_buffer, 8, AT_OK, xMinBlockTime)) return 0;

    return 1;
}

static int esp_check_wifi(void)
{
    memcpy(uart_buffer, "AT+CIPSTATUS\r\n", 14);
    send_to_esp(uart_buffer, 14, NULL, xMinBlockTime);

    if (reset_dma_rx() == HAL_ERROR) return 0;
    if (!wait_uart_rx(xMinBlockTime)) return 0;

    if (check_uart_flag(STA_DISCONNECTED) != NULL) return 0;
    if (check_uart_flag(STA_NO_AP) != NULL) return 0;
    if (check_uart_flag(STA_CONNECTED) != NULL) return 1;
    if (check_uart_flag(STA_GOT_IP) != NULL) return 1;

    return 0;
}

static int esp_connect_wifi(void)
{
    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=1\r\n", 17);
    if (!send_to_esp(uart_buffer, 17, AT_OK, xMinBlockTime)) return 0;

    sprintf(uart_buffer, "AT+CWJAP_DEF=\"%.*s\",\"%.*s\"\r\n", (int)esp_module.sta_ssid_size, esp_module.sta_ssid, (int)esp_module.sta_pass_size, esp_module.sta_pass);
    if (!send_to_esp(uart_buffer, 20 + esp_module.sta_ssid_size + esp_module.sta_pass_size, AT_OK, xMaxBlockTime)) return 0;

    return 1;
}

static int esp_drop_wifi(void)
{
    memcpy(uart_buffer, (uint8_t *)"AT+CWQAP\r\n", 10);
    if (!send_to_esp(uart_buffer, 10, AT_OK, xMinBlockTime)) return 0;

    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=0\r\n", 17);
    if (!send_to_esp(uart_buffer, 17, AT_OK, xMinBlockTime)) return 0;

    return 1;
}

static int esp_start(void)
{
    esp_module.ap_ssid = "AirC Device";
    esp_module.ap_pass = "314159265";
    esp_module.ap_chl = 3;
    esp_module.ap_enc = WPA2_PSK;
    esp_module.ap_max_hosts = 1;
    esp_module.ap_hidden_ssid = 1;

    // Disable AT commands echo
    memcpy(uart_buffer, (uint8_t *)"ATE0\r\n", 6);
    if (!send_to_esp(uart_buffer, 6, AT_OK, xMinBlockTime)) return 0;

    // Set soft AP + station mode
    memcpy(uart_buffer, (uint8_t *)"AT+CWMODE_DEF=3\r\n", 17);
    if (!send_to_esp(uart_buffer, 17, AT_OK, xMinBlockTime)) return 0;

    // Set auto connect to saved wifi network
    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=1\r\n", 17);
    if (!send_to_esp(uart_buffer, 17, AT_OK, xMinBlockTime)) return 0;

    /* Temporary block of code need while button isn't connected */
    esp_drop_wifi();
    /* --- */

    // Set IP for soft AP
    memcpy(uart_buffer, (uint8_t *)"AT+CIPAP_DEF=\"192.168.4.1\"\r\n", 28);
    if (!send_to_esp(uart_buffer, 28, AT_OK, xMinBlockTime)) return 0;

    // Configure soft AP
    sprintf(uart_buffer, "AT+CWSAP_DEF=\"%s\",\"%s\",%d,%d\r\n", 
        esp_module.ap_ssid, esp_module.ap_pass, 
        esp_module.ap_chl, esp_module.ap_enc);
    if (!send_to_esp(uart_buffer, 24 + strlen(esp_module.ap_ssid) + strlen(esp_module.ap_pass), AT_OK, xMinBlockTime)) return 0;

    // Allow multiple TCP connections
    memcpy(uart_buffer, "AT+CIPMUX=1\r\n", 13);
    if (!send_to_esp(uart_buffer, 13, AT_OK, xMinBlockTime)) return 0;

    // Start server if wifi not connected
    if (!esp_check_wifi())
        esp_start_server();

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
        configASSERT(wifi_tsk_handle != NULL);

        if(RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
        {
            __HAL_UART_CLEAR_IDLEFLAG(huart);

            BaseType_t task_woken = pdFALSE;
            vTaskNotifyGiveFromISR(wifi_tsk_handle, &task_woken);
            portYIELD_FROM_ISR(task_woken);
        }
    }     
}