
#include"fans.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

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
void fans_on(){
    HAL_GPIO_WritePin(FIRST_FAN_PORT,FIRST_FAN_PIN,GPIO_PIN_SET);
    HAL_GPIO_WritePin(SECOND_FAN_PORT,SECOND_FAN_PIN,GPIO_PIN_SET);
}

void fans_off(){
    HAL_GPIO_WritePin(FIRST_FAN_PORT,FIRST_FAN_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECOND_FAN_PORT,SECOND_FAN_PIN,GPIO_PIN_RESET);

}
