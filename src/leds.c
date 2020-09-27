#include<stdint.h>
#include "stm32f407xx.h"
#include "main.h"
#include "leds.h"

extern volatile int esp_server_mode;

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
void change_led(LEDs_mode mode)
{
    if(current_mode != mode)
    {
        if(current_mode != OFF_MODE)
        {
            uint16_t active_led = choose_pin(current_mode);
            if (active_led == BLUE_LED) HAL_GPIO_WritePin(GPIOB, active_led, GPIO_PIN_RESET);
            else HAL_GPIO_WritePin(GPIOD, active_led, GPIO_PIN_RESET);
        }
        current_mode = mode;
   }
}

void reed_switch_task(void *pvParams)
{
    /* Notify init task that reed switch task has been started */
    xEventGroupSetBits(eg_task_started, EG_REED_SWITCH_STARTED);
    for( ;; )
    {
        vTaskDelay(1);
        if(HAL_GPIO_ReadPin(GPIOB,REED_SWITCH) == GPIO_PIN_RESET)
        {
            vTaskDelay(reedSwitchHoldInterval);
            if(HAL_GPIO_ReadPin(GPIOB,REED_SWITCH) == GPIO_PIN_RESET)
            {
                change_led(WIFI_MODE);
                if (esp_server_mode == 1)
                    esp_server_mode = 0;
                else
                    esp_server_mode = 1;
            }
        }
    }
}


void initBlueButtonAndReedSwitch()
{
    GPIO_InitTypeDef bluebutton;

    bluebutton.Mode = GPIO_MODE_IT_RISING;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    bluebutton.Pin = BLUE_BUTTON_DISCOVERY;
    HAL_GPIO_Init(GPIOA,&bluebutton);
    __HAL_GPIO_EXTI_CLEAR_IT(BLUE_BUTTON_DISCOVERY);

    HAL_NVIC_SetPriority(EXTI0_IRQn,0,0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    /*Configure GPIO pin : REED_SWITCH */
    GPIO_InitTypeDef reedswitch;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    reedswitch.Pin = REED_SWITCH;
    reedswitch.Mode = GPIO_MODE_INPUT;
    reedswitch.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(GPIOB,&reedswitch);

}

