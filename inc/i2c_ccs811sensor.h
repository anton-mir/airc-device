#ifndef CCS811_H
#define CCS811_H
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2c_ex.h"

#include "FreeRTOS.h"

double get_tvoc(void);
double get_co2(void);

void i2c_ccs811sensor(void *pvParameters);

#endif 

