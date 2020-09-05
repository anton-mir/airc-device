#include"FreeRTOS.h"
#include"task.h"
#include"fans.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include"main.h"
#include"fans.h"
#include"limits.h"

void init_fans(){
    GPIO_InitTypeDef fans_init = {0};

    fans_init.Mode = GPIO_MODE_OUTPUT_PP;
    fans_init.Pin = FIRST_FAN_PIN;
    HAL_GPIO_Init(FIRST_FAN_PORT,&fans_init);
    HAL_GPIO_WritePin(FIRST_FAN_PORT,FIRST_FAN_PIN,GPIO_PIN_RESET);

    fans_init.Mode = GPIO_MODE_OUTPUT_PP;
    fans_init.Pin = SECOND_FAN_PIN;
    HAL_GPIO_Init(SECOND_FAN_PORT,&fans_init);
    HAL_GPIO_WritePin(SECOND_FAN_PORT,SECOND_FAN_PIN,GPIO_PIN_RESET);

}

void fans_control_task(void *pvTaskParams){
   uint32_t fan_switch = 0x0;
   uint32_t ready = (NOTIFICATION_BIT_1); //| NOTIFICATION_BIT_2 | NOTIFICATION_BIT_4)
   for(;;){
        xTaskNotifyWait(0x00,ULONG_MAX,&fan_switch,portMAX_DELAY);
        if(fan_switch  == ready){
            fans_on();
            vTaskDelay(FANS_WORKING_TIME);
            fans_off();
            //unblock pending tasks
            xTaskNotifyGive(uart_sensors_handle);

            // xTaskNotifyGive();
            // xTaskNotifyGive();

        }
    }
}

void fans_on(){
    HAL_GPIO_WritePin(FIRST_FAN_PORT,FIRST_FAN_PIN,GPIO_PIN_SET);
    HAL_GPIO_WritePin(SECOND_FAN_PORT,SECOND_FAN_PIN,GPIO_PIN_SET);
}

void fans_off(){
    HAL_GPIO_WritePin(FIRST_FAN_PORT,FIRST_FAN_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECOND_FAN_PORT,SECOND_FAN_PIN,GPIO_PIN_RESET);

}
