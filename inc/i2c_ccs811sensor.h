#ifndef CCS811_H
#define CCS811_H
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2c_ex.h"

#include "FreeRTOS.h"

struct co2_tvoc {
	double co2, tvoc;
};

struct co2_tvoc get_co2_tvoc(void);
void i2c_ccs811sensor(void *pvParameters);

#endif 

