#ifndef FANS_H
#define FANS_H

#define FIRST_FAN_PIN GPIO_PIN_2
#define FIRST_FAN_PORT GPIOC

#define SECOND_FAN_PIN GPIO_PIN_3
#define SECOND_FAN_PORT GPIOA

#define NOTIFICATION_BIT_1  0x1
#define NOTIFICATION_BIT_2  0x2
#define NOTIFICATION_BIT_4  0x4

#define FANS_WORKING_TIME   2000 //ms

void init_fans();
void fans_control_task(void *pvTaskParams);
void fans_on();
void fans_off();
#endif