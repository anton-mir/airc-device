#include "esp8266_wifi.h"
#include "http_helper.h"
#include "http_defs.h"
#include "hw_delay.h"
#include "main.h"

const uint8_t AT_OK[6] = {0x0D, 0x0A, 0x4F, 0x4B, 0x0D, 0x0A};
const uint8_t IPD_FLAG[5] = {0x2B, 0x49, 0x50, 0x44, 0x2C};

int esp_uart_rx_completed = 0;
int esp_tcp_last_part_flag = 0;
ESP8266 esp_module = { 0 };
ESP8266_TCP_PACKET esp_tcp_packet = { 0 };

UART_HandleTypeDef esp_uart;
DMA_HandleTypeDef esp_dma_rx;

uint8_t uart_buffer[ESP_UART_BUFFER_SIZE];

static int send_command(const char *command)
{
    size_t answer_size = sizeof(AT_OK);
    HAL_UART_Transmit(&esp_uart, (uint8_t *)command, strlen(command), ESP_UART_DELAY);
    esp_uart_rx_completed = 0;
    size_t counter = 0;
    HAL_UART_Receive_IT(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);
    while (!esp_uart_rx_completed && counter < 100)
    {
        if (search_in_buffer(uart_buffer, ESP_UART_BUFFER_SIZE, AT_OK, answer_size) != -1)
        {
            HAL_UART_AbortReceive_IT(&esp_uart);
            HAL_UART_RxCpltCallback(&esp_uart);
        }
        counter++;
        delay_ms(200);
    }
    memset(uart_buffer, 0, ESP_UART_BUFFER_SIZE);

    if (counter == 100) return 0;
    return 1;
}

static int configure_esp(void)
{
    if (!send_command("ATE0\r\n")) return 0;

    char mode_command[17];
    sprintf(mode_command, "AT+CWMODE_CUR=%d\r\n", esp_module.mode);
    if (!send_command(mode_command)) return 0;

    char ap_command[24 + strlen(esp_module.ap_ssid) + strlen(esp_module.ap_pass)];
    sprintf(ap_command, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d\r\n", esp_module.ap_ssid, esp_module.ap_pass, esp_module.ap_chl, esp_module.ap_enc);
    if (!send_command(ap_command)) return 0;

    char mux_command[13];
    sprintf(mux_command, "AT+CIPMUX=%d\r\n", esp_module.mux);
    if (!send_command(mux_command)) return 0;

    char server_command[17 + NUMBER_LENGTH(esp_module.port)];
    sprintf(server_command, "AT+CIPSERVER=1,%d\r\n", esp_module.port);
    if (!send_command(server_command)) return 0;

    return 1;
}

static int connect_sta(void)
{
    char sta_command[20 + strlen(esp_module.sta_ssid) + strlen(esp_module.sta_pass)];
    sprintf(sta_command, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", esp_module.sta_ssid, esp_module.sta_pass);
    if (!send_command(sta_command)) return 0;

    return 1;
}

void wifi_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);

    esp_tcp_packet.ipd_flag = -1;
    esp_uart_rx_completed = 0;
    HAL_UART_Receive_DMA(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);

    esp_uart.hdmarx->XferCpltCallback = ESP_UART_DMAReceiveCpltCallback;
    HAL_DMA_RegisterCallback(&esp_dma_rx, HAL_DMA_XFER_CPLT_CB_ID, esp_uart.hdmarx->XferCpltCallback);

    for (;;)
    {
        vTaskDelay(500);
    }
}

HAL_StatusTypeDef esp_module_init(void)
{
    HAL_StatusTypeDef status;
    /* 
    PINS
    TX - PD5
    RX - PD6
    */
    GPIO_InitTypeDef gpio;
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_OD;
    HAL_GPIO_Init(GPIOD, &gpio);

    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    /* USART 2 */
    esp_uart.Instance = USART2;
    esp_uart.Init.BaudRate = 115200;
    esp_uart.Init.WordLength = UART_WORDLENGTH_8B;
    esp_uart.Init.StopBits = UART_STOPBITS_1;
    esp_uart.Init.Parity = UART_PARITY_NONE;
    esp_uart.Init.Mode = UART_MODE_TX_RX;
    esp_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    esp_uart.Init.OverSampling = UART_OVERSAMPLING_16;

    status = HAL_UART_Init(&esp_uart);
    if (status == HAL_ERROR) return status;

    esp_dma_rx.Instance = DMA1_Stream5;
    esp_dma_rx.Init.Channel = DMA_CHANNEL_4;
    esp_dma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    esp_dma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    esp_dma_rx.Init.MemInc = DMA_MINC_ENABLE;
    esp_dma_rx.Init.Mode = DMA_CIRCULAR;
    esp_dma_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    esp_dma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    esp_dma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    esp_dma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    status = HAL_DMA_Init(&esp_dma_rx);
    if (status == HAL_ERROR) return status;

    __HAL_LINKDMA(&esp_uart, hdmarx, esp_dma_rx);

    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    esp_module.mode = STA_AP;
    esp_module.ap_ssid = "AirC Device";
    esp_module.ap_pass = "314159265";
    esp_module.ap_chl = 5;
    esp_module.ap_enc = WPA2_PSK;
    esp_module.mux = 1;
    esp_module.port = HTTP_SERVER_PORT;

    if (configure_esp() == 0) return HAL_ERROR;

    esp_module.initialized = 1;

    return status;
}

void ESP_UART_DMAReceiveCpltCallback(DMA_HandleTypeDef *_hdma)
{
    if (esp_tcp_last_part_flag) 
        while(uart_buffer[esp_tcp_packet.tcp_length - esp_tcp_packet.tcp_counter - 2] == 0) 
            delay_ms(10);
    
    uint8_t tmp[ESP_UART_BUFFER_SIZE];
    memcpy(tmp, uart_buffer, ESP_UART_BUFFER_SIZE);
    memset(uart_buffer, 0, ESP_UART_BUFFER_SIZE);
    if (esp_tcp_packet.ipd_flag == -1)
    {
        size_t ipd_flag_size = sizeof(IPD_FLAG);
        esp_tcp_packet.ipd_flag = search_in_buffer(tmp, ESP_UART_BUFFER_SIZE, IPD_FLAG, ipd_flag_size);
        if (esp_tcp_packet.ipd_flag != -1)
        {
            esp_tcp_packet.tcp_id = tmp[esp_tcp_packet.ipd_flag + 1] - 0x30;
            char size_str[8];
            memset(size_str, 0, 8);
            for (uint8_t i = 0; i < 8; i++)
            {
                if (tmp[esp_tcp_packet.ipd_flag + 3 + i] == 0x3A) break;
                size_str[i] = tmp[esp_tcp_packet.ipd_flag + 3 + i];
            }
            esp_tcp_packet.tcp_length = atoi(size_str);
            uint8_t offset = esp_tcp_packet.ipd_flag + 4 + NUMBER_LENGTH(esp_tcp_packet.tcp_length);
            esp_tcp_packet.tcp_counter = ESP_UART_BUFFER_SIZE - offset - 1;
            memcpy(esp_tcp_packet.tcp_data, tmp+offset, esp_tcp_packet.tcp_counter);
        }
    }
    else if (esp_tcp_packet.tcp_counter < esp_tcp_packet.tcp_length)
    {
        if (!esp_tcp_last_part_flag)
        {
            uint8_t tcp_temp[esp_tcp_packet.tcp_counter + ESP_UART_BUFFER_SIZE];
            memcpy(tcp_temp, esp_tcp_packet.tcp_data, esp_tcp_packet.tcp_counter);
            memcpy(tcp_temp+esp_tcp_packet.tcp_counter, tmp, ESP_UART_BUFFER_SIZE);
            esp_tcp_packet.tcp_counter += ESP_UART_BUFFER_SIZE;
            memcpy(esp_tcp_packet.tcp_data, tcp_temp, esp_tcp_packet.tcp_counter);
            if (esp_tcp_packet.tcp_counter == esp_tcp_packet.tcp_length)
            {
                esp_tcp_packet.ipd_flag = -1;
                esp_tcp_packet.tcp_counter = 0;
                esp_tcp_packet.tcp_id = 0;
                esp_tcp_packet.tcp_length = 0;
                esp_tcp_last_part_flag = 0;
            }
            else
            {
                if (esp_tcp_packet.tcp_counter + ESP_UART_BUFFER_SIZE > esp_tcp_packet.tcp_length)
                {
                    esp_tcp_last_part_flag = 1;
                }
            }
        }
        else
        {
            size_t tail_size = esp_tcp_packet.tcp_length - esp_tcp_packet.tcp_counter;
            uint8_t tcp_temp[esp_tcp_packet.tcp_counter + tail_size];
            memcpy(tcp_temp, esp_tcp_packet.tcp_data, esp_tcp_packet.tcp_counter);
            memcpy(tcp_temp+esp_tcp_packet.tcp_counter, tmp, tail_size);
            esp_tcp_packet.tcp_counter += tail_size;
            memcpy(esp_tcp_packet.tcp_data, tcp_temp, esp_tcp_packet.tcp_counter);
            
            esp_tcp_packet.ipd_flag = -1;
            esp_tcp_packet.tcp_counter = 0;
            esp_tcp_packet.tcp_id = 0;
            esp_tcp_packet.tcp_length = 0;
            esp_tcp_last_part_flag = 0;
        }
    }
}

int search_in_buffer(uint8_t *buffer, size_t buffer_size, const uint8_t *needle, size_t needle_size)
{
    size_t pos_needle = 0;
    for (size_t i = 0; i < buffer_size - needle_size; ++i)
    {
        if(buffer[i] == needle[pos_needle])
        {
            ++pos_needle;
            if(pos_needle == needle_size)
            {
                return i;
            }
        }
        else
        {
           i -= pos_needle;
           pos_needle = 0;
        }
    }
    return -1;
}