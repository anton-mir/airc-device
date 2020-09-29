
#ifndef NUMBER_HELPER_H
#define NUMBER_HELPER_H

#include "stm32f4xx_hal.h"

#define MAX_PRECISION	    10
#define ULL_STRING_LENGTH   20

char *ftoa(double f, char *buf, int precision);
double atof(char *s);
char *ulltoa(unsigned long long int x, char *buf);
unsigned long long int atoull(char *buf);

#endif //NUMBER_HELPER_H
