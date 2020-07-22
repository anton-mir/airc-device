#include "lm335z.h"
#include "main.h"
#include "task.h"
#include "wh1602.h"
#define BUFFER_TEMP_SIZE (10)


/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc2;
double avg_temp=0;
/**
 * @brief PHY GPIO init
 *
 */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_ADC2_CLK_ENABLE();

}

/**
 * @brief PHY ADC2 init
 *
 */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc2.Init.Resolution = ADC_RESOLUTION_8B;
  hadc2.Init.ScanConvMode = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  { 
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

static void ADC_Init(void)
{
	ADC->CCR |= ADC_CCR_TSVREFE;
	ADC->CCR &= ~ADC_CCR_VBATE;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hadc->Instance==ADC2)
  {
  /* USER CODE BEGIN ADC2_MspInit 0 */

  /* USER CODE END ADC2_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_ADC2_CLK_ENABLE();
  
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC2 GPIO Configuration    
    PB1     ------> ADC2_IN9 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC2_MspInit 1 */

  /* USER CODE END ADC2_MspInit 1 */
  }

}


double Get_Analog_Temp(void)
{
	double temperature=TEMP_ERROR;
	uint32_t sensorValue;
	HAL_ADC_Start(&HAL_ADC);
	if(HAL_ADC_PollForConversion(&HAL_ADC, HAL_TIMEOUT)== HAL_OK)
	{
		sensorValue=HAL_ADC_GetValue(&HAL_ADC);
		temperature = (double)((sensorValue) * VREF / ADC_CONV);
		temperature = (double)((V_TEMP_0 - temperature) * COEF_TEMP);
	}
	HAL_ADC_Stop(&HAL_ADC);
	return (double)temperature;

}

double Get_Avg_Analog_Temp(const uint32_t count)
{
	 if(count==0)
		 return TEMP_ERROR;
	 if(count==1)
		 return Get_Analog_Temp();
	 double sum=0,temp=0;
	 for(uint32_t i=0;i<count;i++)
	 {
		 temp=Get_Analog_Temp();
		 if(temp==TEMP_ERROR)
			 return TEMP_ERROR;
		 sum+=temp;
	 }
	 return (double)sum/count;
}

double get_analog_temp(void)
{
    return avg_temp;
}
 
void analog_temp(void *pvParameters) 
{
    MX_GPIO_Init();
    MX_ADC2_Init();
    ADC_Init();
    HAL_ADC_MspInit(&hadc2);
    xEventGroupSetBits(eg_task_started, EG_ANALOG_TEMP_STARTED);
    double temp=0, buffer_temp[BUFFER_TEMP_SIZE], buffer_avg_temp;
    temp=Get_Analog_Temp();
    avg_temp=temp;
    for(int i=0;i<BUFFER_TEMP_SIZE;i++)
    {
        buffer_temp[i]=temp;
    }
    for (;;)
    {
        temp=Get_Analog_Temp();
        buffer_avg_temp=temp;
        for(int i=BUFFER_TEMP_SIZE-1;i>0;i--)
        {
            buffer_temp[i]=buffer_temp[i-1];
            buffer_avg_temp+=buffer_temp[i];
        }
        buffer_temp[0]=temp;
        buffer_avg_temp/=BUFFER_TEMP_SIZE;
        avg_temp=buffer_avg_temp;
        vTaskDelay(1000);
    }
}


