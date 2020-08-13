#include<stdint.h>
#include"FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include"stm32f407xx.h"
#include "leds.h"
#include "main.h"



LEDs_mode current_mode = WORKING_MODE;

uint16_t choos_right_pin(LEDs_mode mode){
    switch (mode)
    {
        case WORKING_MODE:
            return BLUE_LED;
        case WIFI_MODE:
            return GREEN_LED;
        case FAULT_MODE:
            return RED_LED;
        default:
            return 0;
    }
}

void change_led(LEDs_mode mode){
    if(current_mode != mode){
        HAL_GPIO_WritePin(GPIOD,choos_right_pin(current_mode),GPIO_PIN_RESET);
        current_mode = mode;
   }
}



void init_button(){
    GPIO_InitTypeDef b_init;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    b_init.Pin = GPIO_PIN_0;
    b_init.Mode = GPIO_MODE_IT_RISING;
    HAL_GPIO_Init(GPIOA,&b_init);

    HAL_NVIC_SetPriority(EXTI0_IRQn,0,0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

