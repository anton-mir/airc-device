#ifndef FANS_H
#define FANS_H

#define FIRST_FAN_PIN GPIO_PIN_2
#define FIRST_FAN_PORT GPIOC

#define SECOND_FAN_PIN GPIO_PIN_3
#define SECOND_FAN_PORT GPIOA


#define FANS_WORKING_TIME   3000 //ms

void init_fans();
void fans_on();
void fans_off();
#endif