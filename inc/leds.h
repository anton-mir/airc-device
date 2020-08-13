#ifndef LEDS_H
#define LEDS_H

#include<stdint.h>
#include"FreeRTOS.h"
#include"queue.h"

#define BLUE_LED GPIO_PIN_15
#define RED_LED GPIO_PIN_14
#define GREEN_LED GPIO_PIN_12
#define ORANGE_LED GPIO_PIN_13 //unused


typedef enum {
    WORKING_MODE,
    WIFI_MODE,
    FAULT_MODE
}LEDs_mode;

extern LEDs_mode current_mode;
void init_button();
// extern QueueHandle_t queue_led;
void blink_led();
void change_led(LEDs_mode mode);
uint16_t choos_right_pin(LEDs_mode mode);


#endif
