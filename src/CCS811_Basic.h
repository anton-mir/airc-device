
#ifndef __CCS811_BASIC_H
#define __CCS811_BASIC_H

#include "stm32f4xx_hal.h"

#define CCS811_ADDR 0x5A //7-bit unshifted I2C Address 1
#define CCS811_ADDRD 0x5B //7-bit unshifted I2C Address 2
#define CCS811_ADDWR 0XB6
#define CSS811_STATUS 0x00
#define CSS811_MEAS_MODE 0x01
#define CSS811_ALG_RESULT_DATA 0x02
#define CSS811_RAW_DATA 0x03
#define CSS811_ENV_DATA 0x05
#define CSS811_NTC 0x06
#define CSS811_THRESHOLDS 0x10
#define CSS811_BASELINE 0x11
#define CSS811_HW_ID 0x20
#define CSS811_HW_VERSION 0x21
#define CSS811_FW_BOOT_VERSION 0x23
#define CSS811_FW_APP_VERSION 0x24
#define CSS811_ERROR_ID 0xE0
#define CSS811_APP_START 0xF4
#define CSS811_SW_RESET 0xFF

	//Register addresses
#define FIRST_BASELINE_ADDRESS_VAL              * ((  uint32_t *)(DATA_EEPROM_BASE+2))
#define BURN_IN_TIME_ADDRESS_VAL                * ((  uint32_t *)(DATA_EEPROM_BASE+3))
#define BASELINE_EARLYLIFE_PERIOD_ADDRESS_VAL   * ((  uint32_t *)(DATA_EEPROM_BASE+4))

#define FIRST_BASELINE_ADDRESS             (DATA_EEPROM_BASE+2)
#define BURN_IN_TIME_ADDRESS               (DATA_EEPROM_BASE+3)
#define BASELINE_EARLYLIFE_PERIOD_ADDRESS  (DATA_EEPROM_BASE+4)

#define BURN_IN_TIME              ((48*60*60)/APPLICATION_RUN_CYCLE)//48 Hours
#define RUN_IN_TIME               ((20*60)/APPLICATION_RUN_CYCLE)//20 Minutes
#define NEW_MODE_RUN_IN_TIME      ((10*60)/APPLICATION_RUN_CYCLE)//10 Minutes
#define BASELINE_EARLYLIFE_PERIOD ((500*60*60)/APPLICATION_RUN_CYCLE)//500 Hours
#define BASELINE_EL_STORE_PERIOD  ((24*60*60)/APPLICATION_RUN_CYCLE)//24 Hours
#define BASELINE_AEL_STORE_PERIOD ((7*BASELINE_EL_STORE_PERIOD)/APPLICATION_RUN_CYCLE)
#define CALIB_TEMP_HUM            ((20*60)/APPLICATION_RUN_CYCLE)//20 Minutes

void readResults(void);
void configureCCS811(void);
int checkForError(void);
unsigned int getBaseline(void);
int dataAvailable(void);
int appValid(void);
void enableInterrupts(void);
void disableInterrupts(void);
void setDriveMode(uint8_t mode);
uint8_t readRegister(uint8_t addr);
void writeRegister(uint8_t addr, uint8_t val);
void Init_I2C_CCS811(void);
void softRest(void);
void sleep(void);
void restore_Baseline(void);


#endif  
