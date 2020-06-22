#include "uart_sensors.h"
#include "main.h"

void CO_sensor(void * const arg) {

    /* Notify init task that CO sensor task has been started */
    xEventGroupSetBits(eg_task_started, EG_CO_SENSOR_STARTED);

    for(;;) {
    }
}





