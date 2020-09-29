#include<stdint.h>
#include "CCS811_Basic.h"
#include "i2c_ccs811sensor.h"

#include "main.h"
#include "task.h"
#include "wh1602.h"

struct co2_tvoc co2_tvoc_t;

I2C_HandleTypeDef  hi2cxc;

uint8_t appvalue=0;
uint8_t errvalue=0;
uint8_t mosetting=0;
uint8_t dtvalue =0;	 
uint8_t  Mode_CCS811=1;

static void MX_GPIO_Init(void)
{

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

}

void Init_I2C_CCS811(void)
{
	hi2cxc.Instance = I2C1;
	hi2cxc.Init.ClockSpeed = 100000;
	hi2cxc.Init.OwnAddress1 = 0; 
	hi2cxc.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2cxc.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
	hi2cxc.Init.OwnAddress2 = 0;
	hi2cxc.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
	hi2cxc.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;

	if (HAL_I2C_Init(&hi2cxc) != HAL_OK)
	{
		lcd_print_string("Error: TODO");
		while(1);
	}
#if 0
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2cxc, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		lcd_print_string("Error: TODO");
		while(1); 
	}
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2cxc, 0) != HAL_OK)
	{
		lcd_print_string("Error: TODO");
		while(1); 
	}
#endif

}	

/*
 * @brief  Updates the total voltatile organic compounds (TVOC) in parts per billion (PPB) and the CO2 value.
 * @param  NONE.
 * @retval None.
 */	
void readResults()
{
	uint8_t data_rq[8];
    uint8_t status_sensor = readRegister(0x0);
	HAL_I2C_Mem_Read( &hi2cxc, CCS811_ADDR<<1, ( uint8_t )CSS811_ALG_RESULT_DATA, I2C_MEMADD_SIZE_8BIT, data_rq, 8, 100 );

	uint8_t co2MSB = data_rq[0];
	uint8_t co2LSB = data_rq[1];
	uint8_t tvocMSB = data_rq[2];
	uint8_t tvocLSB = data_rq[3];

	/*	TVOC value, in parts per billion (ppb)
		eC02 value, in parts per million (ppm) */
	co2_tvoc_t.co2 = ((unsigned int)co2MSB << 8) | co2LSB;
	co2_tvoc_t.tvoc = ((unsigned int)tvocMSB << 8) | tvocLSB;
}

/*
 * @brief  configureCCS811.
 * @param  NONE.
 * @retval None.
 */
void configureCCS811()
{
	HAL_Delay(69);

	//Verify the hardware ID is what we expect
	uint8_t hwID = readRegister(0x20); //Hardware ID should be 0x81
	if (hwID != 0x81)
	{
		lcd_print_string("Error: TODO");
		while (1); //Freeze!
	}

	uint8_t lodata[1];

    HAL_I2C_Mem_Write(&hi2cxc, CCS811_ADDR<<1, CSS811_APP_START, I2C_MEMADD_SIZE_8BIT, (uint8_t *) &lodata, 0, 300);

	HAL_Delay(20);
	setDriveMode(Mode_CCS811); //Read every second
	HAL_Delay(10);	

	if (checkForError() == 1)
	{
		lcd_print_string("Error: TODO");
		while (1); //Freeze!
	}
	//Set Drive Mode
}

//Checks to see if error bit is set
int checkForError()
{
	errvalue=readRegister(CSS811_ERROR_ID);
	errvalue = readRegister(CSS811_STATUS);
	return (errvalue & 1 << 0);
}

//Checks to see if DATA_READ flag is set in the status register
int dataAvailable()
{   // dtvalue = readRegister(CSS811_ERROR_ID);
	//HAL_Delay(00);
	dtvalue = readRegister(CSS811_STATUS);
	return (dtvalue & 1 << 3);
}

//Checks to see if APP_VALID flag is set in the status register
int appValid()
{
	appvalue = readRegister(CSS811_STATUS);
	return (appvalue & (1 << 4));
}

//Enable the nINT signal
void enableInterrupts(void)
{
	uint8_t setting = readRegister(CSS811_MEAS_MODE); //Read what's currently there
	setting |= (1 << 3); //Set INTERRUPT bit
	writeRegister(CSS811_MEAS_MODE, setting);
}

//Disable the nINT signal
void disableInterrupts(void)
{
	uint8_t setting = readRegister(CSS811_MEAS_MODE); //Read what's currently there
	setting &= ~(1 << 3); //Clear INTERRUPT bit
	writeRegister(CSS811_MEAS_MODE, setting);
}

/*
 * @brief  //Mode 0 = Idle
//Mode 1 = read every 1s
//Mode 2 = every 10s
//Mode 3 = every 60s
//Mode 4 = RAW mode.
 * @param  MODE.
 * @retval None.
 */
void setDriveMode(uint8_t mode)
{
	if (mode > 4) mode = 4; //Error correction

	mosetting = readRegister(CSS811_MEAS_MODE); //Read what's currently there

	mosetting &=~(7<<4); //Clear DRIVE_MODE bits
	mosetting |= (mode << 4); //Mask in mode

	writeRegister(CSS811_MEAS_MODE, mosetting);
	mosetting = readRegister(CSS811_MEAS_MODE); //Read what's currently there

}

/*
 * @brief  reading_Multiple_Register
 * @param  addr ADDRESS.
 * @param  val VALUE.
 * @param  size SIZE.
 * @retval None.
 */
void read_Mul_Register(uint8_t addr, uint8_t * val,uint8_t size)
{
	HAL_I2C_Mem_Read( &hi2cxc, CCS811_ADDR, ( uint8_t )addr, I2C_MEMADD_SIZE_8BIT, val, size,100 );
}

/*
 * @brief  softRest
 * @param  NONE.
 * @retval None.
 */
void softRest() {
	uint8_t rstCMD[5] = {CSS811_SW_RESET, 0x11,0xE5,0x72,0x8A};

	HAL_I2C_Mem_Write( &hi2cxc, CCS811_ADDWR, CSS811_SW_RESET, I2C_MEMADD_SIZE_8BIT, rstCMD, 5,300);
	while (HAL_I2C_GetState(&hi2cxc) != HAL_I2C_STATE_READY);

}	

/*
 * @brief  sleep
 * @param  NONE.
 * @retval NONE.
 */
void sleep()
{
	// sets sensor to idle; measurements are disabled; lowest power mode
	writeRegister(CSS811_MEAS_MODE, 0);
}

/*
 * @brief  Reads from a give location from the CSS811
 * @param  addr  ADDRESS.
 * @retval VALUE AT THE ADDRESS.
 */	 
uint8_t readRegister(uint8_t addr)
{
	uint8_t dt;
	HAL_StatusTypeDef res;
	res = HAL_I2C_Mem_Read(&hi2cxc, CCS811_ADDR<<1, ( uint8_t )addr,1, &dt, 1, 300 );
	return dt;
}

//Write a value to a spot in the CCS811
/*
 * @brief  Write a value to a spot in the CCS811
 * @param  addr ADDRESS.
 * @param  val  VALUE.
 * @retval NONE.
 */
void writeRegister(uint8_t addr, uint8_t val)
{
	HAL_I2C_Mem_Write( &hi2cxc, CCS811_ADDR<<1, ( uint8_t )addr, I2C_MEMADD_SIZE_8BIT, &val, 1,300);
}


struct co2_tvoc get_co2_tvoc(void)
{
	return co2_tvoc_t;
}

void i2c_ccs811sensor(void *pvParameters) 
{
	MX_GPIO_Init();
	Init_I2C_CCS811();
	configureCCS811();

	xEventGroupSetBits(eg_task_started, EG_I2C_CCS811_STARTED);

	for (;;)
	{
		readResults();	
		vTaskDelay(1000);
	}
}


