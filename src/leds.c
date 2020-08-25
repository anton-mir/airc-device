#include<stdint.h>
#include"FreeRTOS.h"
#include"task.h"
#include "leds.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"



LEDs_mode current_mode = WORKING_MODE;
const uint16_t reedSwitchHoldInterval = 3000;

uint16_t choose_pin(LEDs_mode mode){
    switch (mode)
    {
        case WORKING_MODE:
            return BLUE_LED;
        case WIFI_MODE:
            return GREEN_LED;
        case FAULT_MODE:
            return RED_LED;
        case OFF_MODE:
            return OFF_LEDS;
        default:
            return BLUE_LED;
    }
}
/*
    init_task() will blink LED that depends on some mode
    the mode is defined in current_mode variable
    change_led() change the current_mode variable
*/
void change_led(LEDs_mode mode){
    if(current_mode != mode){
        if(current_mode != OFF_MODE) {
            uint16_t active_led = choose_pin(current_mode);
            if (active_led == BLUE_LED) HAL_GPIO_WritePin(GPIOB, active_led, GPIO_PIN_RESET);
            else HAL_GPIO_WritePin(GPIOD, active_led, GPIO_PIN_RESET);
        }
        current_mode = mode;
   }
}

void reed_switch_task(void *pvParams){
    for( ;; ){
        vTaskDelay(1);
        if(HAL_GPIO_ReadPin(GPIOB,REED_SWITCH) == GPIO_PIN_SET){
            vTaskDelay(reedSwitchHoldInterval);
            if(HAL_GPIO_ReadPin(GPIOB,REED_SWITCH) == GPIO_PIN_SET){
                change_led(WIFI_MODE);
            }
        }
    }
}



void init_button(){
    GPIO_InitTypeDef b_init;

    b_init.Mode = GPIO_MODE_IT_RISING;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    b_init.Pin = BLUE_BUTTON_DISCOVERY;
    HAL_GPIO_Init(GPIOA,&b_init);
    __HAL_GPIO_EXTI_CLEAR_IT(BLUE_BUTTON_DISCOVERY);

    HAL_NVIC_SetPriority(EXTI0_IRQn,0,0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    /*Configure GPIO pin : REED_SWITCH */
    GPIO_InitTypeDef reedsw_init;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    reedsw_init.Pin = REED_SWITCH;
    reedsw_init.Mode = GPIO_MODE_INPUT;
    reedsw_init.Pull = GPIO_PULLDOWN;

    HAL_GPIO_Init(GPIOB,&reedsw_init);

}

