#include <memory.h>
#include <stdio.h>
#include <limits.h>
#include "math.h"
#include "esp8266_wifi.h"
#include "http_helper.h"
#include "picohttpparser.h"
#include "main.h"

#include "wh1602.h"

static struct ESP8266 esp_module = { 0 }; // ESP8266 init struct
volatile int esp_server_mode = 0;

UART_HandleTypeDef esp_uart = { 0 };
DMA_HandleTypeDef esp_dma_rx = { 0 };
DMA_HandleTypeDef esp_dma_tx = { 0 };

static size_t uart_data_size = 0;
static uint8_t uart_buffer[ESP_UART_BUFFER_SIZE];

static struct ESP8266_TCP_PACKET tcp_packet = { 0 };

static struct phr_header http_headers[HTTP_MAX_HEADERS];

static size_t get_uart_data_length();
static void reset_dma_rx();

static uint32_t esp_send_data(uint8_t *data, size_t data_size);
static uint32_t esp_start(void);

void esp_rx_task(void * const arg)
{
    char *pos;

    xEventGroupSetBits(eg_task_started, EG_ESP_RX_TSK_STARTED);

    for (;;)
    {
        HAL_UART_DMAStop(&esp_uart);
        HAL_UART_Receive_DMA(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        uart_data_size = ESP_UART_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&esp_dma_rx);

        if (strstr((char *)uart_buffer, "\r\nOK\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_COMMAND_OK, eSetValueWithOverwrite);
        }
        else if (strstr((char *)uart_buffer, "\r\nSEND OK\r\n") != NULL)
        {
            xTaskNotify(wifi_tsk_handle, ESP_SEND_OK, eSetValueWithOverwrite);
        }
        else if ((pos = strstr((char *)uart_buffer, "+IPD,")) != NULL)
        {
            char str[16];
            memcpy(str, pos, 16);
            lcd_clear();
            lcd_print_string(str);
            if (tcp_packet.status == 0)
            {
                int res = sscanf(pos, "+IPD,%d,%d:", &tcp_packet.id, &tcp_packet.length);
                if (res == 2)
                {
                    memcpy(tcp_packet.data, pos + 9 + NUMBER_LENGTH(tcp_packet.length), tcp_packet.length);
                    tcp_packet.status = 1; 
                }
            }
        }
    }
}

void wifi_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);

    esp_start();
    
    for (;;)
    {
        if (tcp_packet.status == 1)
        {
            sprintf((char *)uart_buffer, "AT+CIPSEND=%d,5\r\n", tcp_packet.id);
            esp_send_data(uart_buffer, 16);
            tcp_packet.status = 2;
        }
        vTaskDelay(500);
    }
}

static uint32_t esp_send_data(uint8_t *data, size_t data_size)
{
    HAL_UART_Transmit(&esp_uart, data, data_size, ESP_UART_DELAY);

    xTaskNotifyStateClear(wifi_tsk_handle);
    uint32_t status = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    
    return status;
}

static uint32_t esp_start(void)
{
    esp_module.ap_ssid = "AirC Device";
    esp_module.ap_pass = "314159265";
    esp_module.ap_chl = 1;
    esp_module.ap_enc = WPA2_PSK;

    // Disable AT commands echo
    memcpy(uart_buffer, (uint8_t *)"ATE0\r\n", 6);
    if (esp_send_data(uart_buffer, 6) == ESP_COMMAND_OK);
    else return 0;

    // Configure command for getting list of AP's
    memcpy(uart_buffer, (uint8_t *)"AT+CWLAPOPT=0,15\r\n", 18);
    if (esp_send_data(uart_buffer, 18) == ESP_COMMAND_OK);
    else return 0;

    // Set soft AP + station mode
    memcpy(uart_buffer, (uint8_t *)"AT+CWMODE_DEF=3\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_COMMAND_OK);
    else return 0;

    // Set auto connect to saved wifi network
    memcpy(uart_buffer, (uint8_t *)"AT+CWAUTOCONN=1\r\n", 17);
    if (esp_send_data(uart_buffer, 17) == ESP_COMMAND_OK);
    else return 0;

    // Set IP for soft AP
    memcpy(uart_buffer, (uint8_t *)"AT+CIPAP_DEF=\"192.168.4.1\"\r\n", 28);
    if (esp_send_data(uart_buffer, 28) == ESP_COMMAND_OK);
    else return 0;

    // Configure soft AP
    sprintf((char *)uart_buffer, "AT+CWSAP_DEF=\"%s\",\"%s\",%d,%d,5,1\r\n", 
        esp_module.ap_ssid, esp_module.ap_pass, 
        esp_module.ap_chl, esp_module.ap_enc);
    if (esp_send_data(uart_buffer, 28 + strlen(esp_module.ap_ssid) + strlen(esp_module.ap_pass)) == ESP_COMMAND_OK);
    else return 0;

    // Allow multiple TCP connections
    memcpy(uart_buffer, "AT+CIPMUX=1\r\n", 13);
    if (esp_send_data(uart_buffer, 13) == ESP_COMMAND_OK);
    else return 0;

    // Start server
    sprintf((char *)uart_buffer, "AT+CIPSERVER=1,%d\r\n", HTTP_SERVER_PORT);
    if (esp_send_data(uart_buffer, 17 + NUMBER_LENGTH(HTTP_SERVER_PORT)) == ESP_COMMAND_OK);
    else return 0;

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