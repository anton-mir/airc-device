#ifndef LEDS_H
#define LEDS_H

#include<stdint.h>

#define BLUE_LED    GPIO_PIN_14 // PB
#define RED_LED     GPIO_PIN_3 // PD
#define GREEN_LED   GPIO_PIN_2 // PD

#define BLUE_LED_DISCOVERY    GPIO_PIN_15
#define RED_LED_DISCOVERY     GPIO_PIN_14
#define GREEN_LED_DISCOVERY   GPIO_PIN_12
#define ORANGE_LED_DISCOVERY  GPIO_PIN_13 //unused

#define OFF_LEDS    0xFF

#define BLUE_BUTTON_DISCOVERY GPIO_PIN_0
#define REED_SWITCH GPIO_PIN_15 // PB

typedef enum {
    WORKING_MODE=1,
    WIFI_MODE,
    FAULT_MODE,
    OFF_MODE
}LEDs_mode;

extern LEDs_mode current_mode;

void initBlueButtonAndReedSwitch();
void blink_led();
void change_led(LEDs_mode mode);
uint16_t choose_pin(LEDs_mode mode);
void reed_switch_task(void *pvParams);

#endif
