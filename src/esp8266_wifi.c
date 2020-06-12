#include "esp8266_wifi.h"
#include "wh1602.h"
#include "hw_delay.h"
#include "main.h"

void wifi_task(void * const arg)
{
    xEventGroupSetBits(eg_task_started, EG_WIFI_TSK_STARTED);

    for (;;)
    {
        HAL_UART_Transmit(&esp_uart, (uint8_t*)"AT+RST\r\n", 8, HAL_MAX_DELAY);
        vTaskDelay(10000);
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
    
    return status;
}