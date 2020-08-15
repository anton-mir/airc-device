#ifndef LEDS_H
#define LEDS_H

#include<stdint.h>


#define BLUE_LED    GPIO_PIN_15
#define RED_LED     GPIO_PIN_14
#define GREEN_LED   GPIO_PIN_12
#define ORANGE_LED  GPIO_PIN_13 //unused
#define OFF_LEDS    0xFF

#define BLUE_BUTTON GPIO_PIN_0


typedef enum {
    WORKING_MODE=1,
    WIFI_MODE,
    FAULT_MODE,
    OFF_MODE
}LEDs_mode;

extern LEDs_mode current_mode;

void init_button();
void blink_led();
void change_led(LEDs_mode mode);
uint16_t choose_pin(LEDs_mode mode);


#endif
