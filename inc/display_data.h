#ifndef DISPLAY_DATA_H
#define DISPLAY_DATA_H

#define DISPLAY_TIME_PERIOD 2000//ms
void display_data_task(void *pvParams);

extern xQueueHandle displayQueueHandle;
#endif