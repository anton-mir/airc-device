#include "esp8266_wifi.h"
#include "wh1602.h"
#include "hw_delay.h"
#include "main.h"

static ESP8266 esp_module = { 0 };

static void send_command(const char *command, size_t command_size, size_t answer_size)
{
    HAL_UART_Transmit_IT(&esp_uart, (uint8_t *)command, command_size);
}

void wifi_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);

    for (;;)
    {
        if (esp_module.initialized)
        {
            send_command("AT+CIPSTATUS\r\n", 14, 0);
            delay_s(2);
            send_command("AT+CIPSEND=0,101\r\n", 18, 0);
            delay_s(2);
            send_command("HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: 41\n\n<h1>Hello from AirC device via wifi!</h1>", 101, 0);
            delay_s(2);        
        }
        vTaskDelay(500);
    }
}

HAL_StatusTypeDef esp_module_init(void)
{
    GPIO_InitTypeDef gpio;
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_AF_OD;
    HAL_GPIO_Init(GPIOA, &gpio);

    esp_uart.Instance = USART2;
    esp_uart.Init.BaudRate = 115200;
    esp_uart.Init.WordLength = UART_WORDLENGTH_8B;
    esp_uart.Init.StopBits = UART_STOPBITS_1;
    esp_uart.Init.Parity = UART_PARITY_NONE;
    esp_uart.Init.Mode = UART_MODE_TX_RX;
    esp_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    esp_uart.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_StatusTypeDef status = HAL_UART_Init(&esp_uart);

    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    send_command("AT\r\n", 4, 0);
    delay_s(2);
    send_command("AT+CWMODE_CUR=3\r\n", 17, 0);
    delay_s(2);
    send_command("AT+CWJAP_CUR=\"Orion XXX\",\"61638498\"\r\n", 37, 0);
    delay_s(2);
    send_command("AT+CWSAP_CUR=\"AirC Device\",\"314159265\",5,3\r\n", 44, 0);
    delay_s(2);
    send_command("AT+CIPMODE=0\r\n", 14, 0);
    delay_s(2);
    send_command("AT+CIPMUX=1\r\n", 13, 0);
    delay_s(2);
    send_command("AT+CIPSERVER=1,1001\r\n", 21, 0);
    delay_s(2);

    esp_module.initialized = 1;

    return status;
}