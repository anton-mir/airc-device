//
// Created by dmytro on 16.06.20.
//

#include "uart_sensors.h"

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"


#define BUF_LEN                         (1024)
#define ECHO_SERVER_PORT                (11333)
#define MAX_CLIENTS                     (4)
#define BUF_FOR_SDS_DATA                (50)
static uint8_t buf[BUF_LEN];
static uint8_t data[BUF_FOR_SDS_DATA];

SDS sds;
uint16_t size = 0;


static void USART3_UART_Init(void)
{

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 9600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;


    if (HAL_HalfDuplex_Init(&huart3) == HAL_OK)
    {
        strncpy(buf, "UART initialized\n", 17);
    }
}

static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __USART3_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    HAL_NVIC_SetPriority(USART3_IRQn, 2, 0U);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6|GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

static void Set_CO_RX(void){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
}

static void Set_CO_TX(void){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
}

static void Set_SDS_RX(void){
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
}
void echo_server(void * const arg)
{
    int listenfd;
    fd_set readfds;
    int maxfd;
    int err;
    int read_len;
    struct sockaddr_in server_addr;
    struct sockaddr_storage client_addr;
    socklen_t client_len;
    struct netif *netif = (struct netif *)arg;

    memset(&server_addr, 0, sizeof(server_addr));

    /* Notify init task that echo server task has been started */
    xEventGroupSetBits(eg_task_started, EG_ECHO_SERVER_STARTED);

    listenfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    LWIP_ASSERT("echo_server(): Socket create failed", listenfd >= 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
    server_addr.sin_port = lwip_htons(ECHO_SERVER_PORT);

    err = lwip_bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err < 0) {
        LWIP_ASSERT("echo_server(): Socket bind failed", 0);
    }

    err = lwip_listen(listenfd, MAX_CLIENTS);
    if (err < 0) {
        LWIP_ASSERT("echo_server(): Socket listen failed", 0);
    }

    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);
    maxfd = listenfd;

    for (;;)
    {
        err = select((maxfd + 1), &readfds, NULL, NULL, NULL);
        if (err < 0)
            continue;

        for (int fd = 0; fd <= maxfd; ++fd)
        {
            if (FD_ISSET(fd, &readfds))
            {
                if (fd == listenfd)
                {
                    int clientfd = lwip_accept(
                            listenfd,
                            (struct sockaddr *)&client_addr,
                            &client_len);
                    if (clientfd >= 0)
                    {
                        FD_SET(clientfd, &readfds);
                        if (clientfd > maxfd)
                        {
                            maxfd = clientfd;
                        }
                    }
                }
                else
                {
                    read_len = strlen(buf);
                    if (read_len <= 0)
                    {
                        lwip_close(fd);
                        FD_CLR(fd, &readfds);
                    }
                    else
                    {
                        /* send echo */
                        lwip_write(fd, buf, read_len);
                    }
                }
            }
        }
    }
}

void CO_sensor(void * const arg) {
    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_CO_SENSOR_STARTED);

    GPIO_Init();
    USART3_UART_Init();

    Set_CO_RX();
    uint8_t command = 'c';
    HAL_UART_Transmit(&huart3, (uint8_t *) command, 1, 0xFFFF);
    //while (HAL_UART_GetState(&huart3) == HAL_UART_STATE_BUSY_TX);
    HAL_Delay(100);
    HAL_UART_Transmit(&huart3, (uint8_t *) command, 1, 0xFFFF);
    //while (HAL_UART_GetState(&huart3) == HAL_UART_STATE_BUSY_TX);
    HAL_Delay(100);
    command = '5';
    HAL_UART_Transmit(&huart3, (uint8_t *) command, 1, 0xFFFF);
    //while (HAL_UART_GetState(&huart3) == HAL_UART_STATE_BUSY_TX);

    Set_CO_TX();
    uint8_t co_data[512] = {0};
    co_data[511] = '\0';
    HAL_UART_Receive_IT(&huart3, (uint8_t*) co_data, 511);
    for(;;) {
        if (huart3.RxXferCount == 0) {
            co_data[511] = '\0';
            HAL_UART_Receive_IT(&huart3, (uint8_t*) co_data, 511);
            strcpy(buf, co_data);//TODO: change to strncpy
            co_data[511] = '\0';
        }
        vTaskDelay(500);
    }
}

///// SDS011 sensor funcs
void sdsInit(SDS* sds, const UART_HandleTypeDef* huart_sds) {
    sds->huart3=(UART_HandleTypeDef *)huart_sds;
    HAL_UART_Transmit(sds->huart3,(uint8_t*)Sds011_WorkingMode, 19,30);
    HAL_UART_Receive_IT(sds->huart3, sds->data_receive, 10);
}

int8_t sdsWorkingMode(SDS* sds) {
    return HAL_UART_Transmit(sds->huart3, (uint8_t*)Sds011_WorkingMode,19,30)==HAL_OK ? 1:0;
}

int8_t sdsSleepMode(SDS* sds) {
    return HAL_UART_Transmit(sds->huart3, (uint8_t*)Sds011_SleepCommand,19,30)==HAL_OK ? 1:0;
}

int8_t sdsSend(SDS* sds, const uint8_t* data_buffer, const uint8_t length) {
    return HAL_UART_Transmit(sds->huart3,(uint8_t *) data_buffer,length,30)==HAL_OK ? 1:0;
}

uint16_t sdsGetPm2_5(SDS* sds) {
    return  sds->pm_2_5;
}

uint16_t sdsGetPm10(SDS* sds) {
    return  sds->pm_10;
}

void sds_uart_RxCpltCallback(SDS* sds, UART_HandleTypeDef *huart) {
    if(huart == sds->huart3)
    {
        if((sds->data_receive[1] == 0xC0))
        {
            sds->pm_2_5 = ((sds->data_receive[3]<<8)| sds->data_receive[2])/10;
            sds->pm_10 = ((sds->data_receive[5]<<8)| sds->data_receive[4])/10;
        }
        HAL_UART_Receive_IT(sds->huart3, sds->data_receive, 10);
    }
}

void SDS_sensor(void * const arg) {
    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_SDS_SENSOR_STARTED);

    GPIO_Init();
    USART3_UART_Init();
    Set_SDS_RX();

    sdsInit(&sds, &huart3);


    while (1) {
        size = sprintf(data, "%d %d \n\r", sdsGetPm2_5(&sds), sdsGetPm10(&sds));
        HAL_UART_Transmit_IT(&huart3, data, size);
        sds_uart_RxCpltCallback(&sds, &huart3);
        HAL_Delay(1000);
    }
}



