#include "esp8266_wifi.h"
#include "hw_delay.h"
#include "http_helper.h"
#include "wh1602.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"

const uint8_t AT_OK[6] = {0x0D, 0x0A, 0x4F, 0x4B, 0x0D, 0x0A};
const uint8_t SEND_OK[11] = {0x0D, 0x0A, 0x53, 0x45, 0x4E, 0x44, 0x20, 0x4F, 0x4B, 0x0D, 0x0A};
const uint8_t IPD_FLAG_START[5] = {0x2B, 0x49, 0x50, 0x44, 0x2C};
const uint8_t IPD_FLAG_END[1] = {0x3A};

ESP8266 esp_module = { 0 };
ESP8266_TCP_PACKET rx_tcp_packet = { 0 };
ESP8266_TCP_QUAUE tcp_quaue = { 0 };

UART_HandleTypeDef esp_uart;
DMA_HandleTypeDef esp_dma_rx;
volatile int esp_uart_rx_completed;

static SemaphoreHandle_t sem_wifi_task;

int ipd_pos = 0;
int tcp_pos = 0;
int esp_tcp_tail = 0;

char *command;
uint8_t *tmp;
size_t rx_tcp_counter;

static uint8_t uart_buffer[ESP_UART_BUFFER_SIZE];

static int check_uart_flag(const uint8_t *flag);
static int send_command(const char *command);
static int configure_esp(void);
static int connect_sta(void);
static int send_tcp_data(ESP8266_TCP_PACKET packet);
static int search_in_buffer(uint8_t *buffer, size_t buffer_size, const uint8_t *needle, size_t needle_size);

static int check_uart_flag(const uint8_t *flag)
{
    esp_uart_rx_completed = 0;
    size_t answer_size = strlen(flag);
    size_t counter = 0;
    HAL_UART_Receive_IT(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);
    while (!esp_uart_rx_completed && counter < 10000)
    {
        if (search_in_buffer(uart_buffer, ESP_UART_BUFFER_SIZE, flag, answer_size) != -1)
        {
            HAL_UART_AbortReceive_IT(&esp_uart);
            HAL_UART_RxCpltCallback(&esp_uart);
        }
        counter++;
        delay_ms(1);
    }

    if (counter == 10000) return 0;
    else return 1;
}

static int send_command(const char *command)
{
    HAL_UART_Transmit(&esp_uart, (uint8_t *)command, strlen(command), ESP_UART_DELAY);
    if (check_uart_flag(AT_OK)) return 1;
    else return 0;
}

static int configure_esp(void)
{
    if (!send_command("ATE1\r\n")) return 0;

    command = (char *)pvPortMalloc(17);
    sprintf(command, "AT+CWMODE_CUR=%d\r\n", esp_module.mode);
    if (!send_command(command)) 
    {
        vPortFree(command);
        return 0;
    }
    vPortFree(command);

    command = (char *)pvPortMalloc(24 + strlen(esp_module.ap_ssid) + strlen(esp_module.ap_pass));
    sprintf(command, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d\r\n", esp_module.ap_ssid, esp_module.ap_pass, esp_module.ap_chl, esp_module.ap_enc);
    if (!send_command(command))
    {
        vPortFree(command);
        return 0;
    }
    vPortFree(command);

    command = (char *)pvPortMalloc(13);
    sprintf(command, "AT+CIPMUX=%d\r\n", esp_module.mux);
    if (!send_command(command))
    {
        vPortFree(command);
        return 0;
    }
    vPortFree(command);

    command = (char *)pvPortMalloc(17 + NUMBER_LENGTH(esp_module.port));
    sprintf(command, "AT+CIPSERVER=1,%d\r\n", esp_module.port);
    if (!send_command(command))
    {
        vPortFree(command);
        return 0;
    }
    vPortFree(command);

    return 1;
}

static int connect_sta(void)
{
    command = (char *)pvPortMalloc(20 + strlen(esp_module.sta_ssid) + strlen(esp_module.sta_pass));
    sprintf(command, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", esp_module.sta_ssid, esp_module.sta_pass);
    if (!send_command(command))
    {
        vPortFree(command);
        return 0;
    }
    vPortFree(command);

    return 1;
}

static int send_tcp_data(ESP8266_TCP_PACKET packet)
{
    if (packet.length <= ESP_MAX_TCP_SIZE)
    {
        command = (char *)pvPortMalloc(15 + NUMBER_LENGTH(packet.length));
        sprintf(command, "AT+CIPSEND=%d,%d\r\n", packet.id, packet.length);
        if (!send_command(command))
        {
            vPortFree(command);
            return -1;
        }
        vPortFree(command);

        HAL_UART_Transmit(&esp_uart, packet.data, packet.length, ESP_UART_DELAY);
        if (!check_uart_flag(SEND_OK)) return -1;
    }
    else
    {
        size_t packets_count = packet.length / ESP_MAX_TCP_SIZE;
        for (uint16_t i = 0; i < packets_count; i++)
        {
            command = (char *)pvPortMalloc(15 + NUMBER_LENGTH(ESP_MAX_TCP_SIZE));
            sprintf(command, "AT+CIPSEND=%d,%d\r\n", packet.id, ESP_MAX_TCP_SIZE);
            if (!send_command(command))
            {
                vPortFree(command);
                return -1;
            }
            vPortFree(command);

            HAL_UART_Transmit(&esp_uart, packet.data+(i*ESP_MAX_TCP_SIZE), ESP_MAX_TCP_SIZE, ESP_UART_DELAY);
            if (!check_uart_flag(SEND_OK)) return -1;
        }
        if ((packets_count * ESP_MAX_TCP_SIZE) < packet.length)
        {
            size_t tail = packet.length - (packets_count * ESP_MAX_TCP_SIZE);
            command = (char *)pvPortMalloc(15 + NUMBER_LENGTH(tail));
            sprintf(command, "AT+CIPSEND=%d,%d\r\n", packet.id, tail);
            if (!send_command(command))
            {
                vPortFree(command);
                return -1;
            }
            vPortFree(command);

            HAL_UART_Transmit(&esp_uart, packet.data+(packets_count * ESP_MAX_TCP_SIZE), tail, ESP_UART_DELAY);
            if (!check_uart_flag(SEND_OK)) return -1;
        }
    }
    
    return packet.length;
}

static void rx_uart_dma_start(void)
{
    HAL_UART_Receive_DMA(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);
    esp_uart.hdmarx->XferCpltCallback = ESP_UART_DMAReceiveCpltCallback;
    HAL_DMA_RegisterCallback(&esp_dma_rx, HAL_DMA_XFER_CPLT_CB_ID, esp_uart.hdmarx->XferCpltCallback);
}

static void clear_tcp_packet(void)
{
    rx_tcp_packet.id = 0;
    rx_tcp_packet.length = 0;
    rx_tcp_packet.buffer_size = 0;
    vPortFree(rx_tcp_packet.data);
}

void wifi_task(void * const arg)
{
    sem_wifi_task = xSemaphoreCreateBinary();
    configASSERT(sem_wifi_task != NULL);

    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);

    rx_uart_dma_start();
    for (;;)
    {
        if (xSemaphoreTake(sem_wifi_task, portMAX_DELAY) == pdTRUE)
        {
            if (rx_tcp_packet.data == NULL)
                rx_tcp_packet.data = (uint8_t *)pvPortMalloc(rx_tcp_packet.length);

            memcpy(rx_tcp_packet.data+rx_tcp_counter, uart_buffer+tcp_pos, rx_tcp_packet.buffer_size);
            rx_tcp_counter += rx_tcp_packet.buffer_size;

            if (rx_tcp_counter + ESP_UART_BUFFER_SIZE > rx_tcp_packet.length)
                esp_tcp_tail = rx_tcp_packet.length - rx_tcp_counter;

            if (rx_tcp_packet.length == rx_tcp_counter)
            {
                HTTP_REQUEST request = { 0 };
                uint16_t code = http_parse_request(&request, rx_tcp_packet.data);
                
                rx_tcp_counter = 0;
                clear_tcp_packet();
            }
        }
        
        
        /*if (response.code != 0)
        {
            HAL_UART_DMAStop(&esp_uart);
            format_http_response(&response);

            char http_data[strlen(response.http) + strlen(response.content_type_header) + strlen(response.content_length_header) + NUMBER_LENGTH(response.content_size) + response.content_size + 4];
            sprintf(http_data, "%s\n%s\n%s%d\n\n%s", response.http, response.content_type_header, response.content_length_header, response.content_size, response.body);
            send_tcp_data(http_data);

            HAL_UART_Receive_DMA(&esp_uart, uart_buffer, ESP_UART_BUFFER_SIZE);
            esp_uart.hdmarx->XferCpltCallback = ESP_UART_DMAReceiveCpltCallback;
            HAL_DMA_RegisterCallback(&esp_dma_rx, HAL_DMA_XFER_CPLT_CB_ID, esp_uart.hdmarx->XferCpltCallback);

            response.code = 0;
            response.content_size = 0;
            response.content_type = -1;
            response.id = 0;
            response.body_template = -1;
            response.body = "";
            response.content_type_header = "";
            response.content_length_header = "";
            response.http = "";
        }*/
        //vTaskDelay(NULL);
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

    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, ESP_INT_PRIO, 0);
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
    ipd_pos = search_in_buffer(uart_buffer, ESP_UART_BUFFER_SIZE, IPD_FLAG_START, sizeof(IPD_FLAG_START));
    if (ipd_pos >= 0)
    {
        rx_tcp_packet.id = uart_buffer[ipd_pos + 1] - 0x30;
        tcp_pos = search_in_buffer(uart_buffer, ESP_UART_BUFFER_SIZE, IPD_FLAG_END, sizeof(IPD_FLAG_END));
        if (tcp_pos >= 0)
        {
            uint8_t size = tcp_pos - ipd_pos - 3;
            uint8_t length_str[size];
            memcpy(length_str, uart_buffer+ipd_pos+3, size);
            rx_tcp_packet.length = atoi((char *)length_str);

            tcp_pos++;
            rx_tcp_packet.buffer_size = ESP_UART_BUFFER_SIZE - tcp_pos;
        }
    }
    else 
    {
        ipd_pos = 0;
        tcp_pos = 0;
        
        if (esp_tcp_tail > 0)
        {
            rx_tcp_packet.buffer_size = esp_tcp_tail;
            esp_tcp_tail = 0;
        }
        else
            rx_tcp_packet.buffer_size = ESP_UART_BUFFER_SIZE;
    }

    if (rx_tcp_packet.length > 0)
    {
        if (sem_wifi_task != NULL)
        {
            BaseType_t reschedule = pdFALSE;
            xSemaphoreGiveFromISR(sem_wifi_task, &reschedule);
            portYIELD_FROM_ISR(reschedule);
        }
    }
}

/*void ESP_UART_DMAReceiveCpltCallback(DMA_HandleTypeDef *_hdma)
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
                parse_http_request(&request);
                if (request.status != 500)
                {
                    response = build_http_response(request, 200, esp_tcp_packet.tcp_id);
                }
                else
                {
                    response = build_http_response(request, 500, esp_tcp_packet.tcp_id);
                }

                request.body = "";
                request.body_length = 0;
                request.data_type = -1;
                request.method = -1;
                request.path = "";
                request.path_length = 0;
                request.status = 0;
                esp_tcp_packet.ipd_flag = -1;
                esp_tcp_packet.tcp_counter = 0;
                esp_tcp_packet.tcp_id = 0;
                esp_tcp_packet.tcp_length = 0;
                memset(esp_tcp_packet.tcp_data, 0, 1);
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
            
            parse_http_request(&request);
            if (request.status != 500)
            {
                response = build_http_response(request, 200, esp_tcp_packet.tcp_id);
            }
            else
            {
                response = build_http_response(request, 500, esp_tcp_packet.tcp_id);
            }

            request.body = "";
            request.body_length = 0;
            request.data_type = -1;
            request.method = -1;
            request.path = "";
            request.path_length = 0;
            request.status = 0;
            esp_tcp_packet.ipd_flag = -1;
            esp_tcp_packet.tcp_counter = 0;
            esp_tcp_packet.tcp_id = 0;
            esp_tcp_packet.tcp_length = 0;
            memset(esp_tcp_packet.tcp_data, 0, 1);
            esp_tcp_last_part_flag = 0;
        }
    }
}

void parse_http_request(HTTP_REQUEST *request)
{
    struct phr_header headers[HTTP_MAX_HEADERS];
    size_t headers_count = HTTP_MAX_HEADERS;
    char *method;
    size_t method_length;
    int version;
    int res = phr_parse_request(esp_tcp_packet.tcp_data, esp_tcp_packet.tcp_length, 
                                &method, &method_length, 
                                &request->path, &request->path_length, &version, 
                                &headers, &headers_count);
    
    if (res < 0)
    {
        request->status = 200;
        return;
    }
    else 
    {
        if (memcmp(method, "GET", 3) == 0) request->method = GET;
        else if (memcmp(method, "POST", 4) == 0) request->method = POST;
        else if (memcmp(method, "PUT", 3) == 0) request->method = PUT;
        else if (memcmp(method, "DELETE", 6) == 0) request->method = DELETE;
        else 
        {
            request->status = 500;
            return;
        }
    }
    
    for (uint8_t i = 0; i < headers_count; i++)
    {
        if (memcmp(headers[i].name, "Content-Type", 13) == 0)
        {
            if (memcmp(headers[i].value, "application/json", 16) == 0)
                request->data_type = JSON_DATA;
            else if (memcmp(headers[i].value, "multipart/form-data", 19) == 0)
                request->data_type = FORM_DATA;
            else request->data_type = TEXT_DATA;

            break;
        }
    }

    request->body_length = esp_tcp_packet.tcp_length - res - 1;
    if (request->body_length > 0)
        request->body = esp_tcp_packet.tcp_data+res;
}*/

static int search_in_buffer(uint8_t *buffer, size_t buffer_size, const uint8_t *needle, size_t needle_size)
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