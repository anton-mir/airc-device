#ifndef LM335Z_H
#define LM335Z_H
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"

/* ################### LM335Z configuration ###################### */
#define VREF 		3000 // in mV
#define ADC_CONV 	255
#define V_TEMP_0    2020.0L // in mV
#define HAL_ADC     hadc2
#define COEF_TEMP	0.05L
#define TEMP_ERROR  -273
#define HAL_TIMEOUT 100



void analog_temp(void *pvParameters);

#endif /* LM335Z_H */

