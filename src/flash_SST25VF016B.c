#include "flash_SST25VF016B.h"
#include "stm32f4xx_hal.h"

SPI_HandleTypeDef hspi1;

void Flash_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);

    /*Configure GPIO pin : PD7 */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
    WriteToStatusRegister(0);
}

void SendByte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi1, &data, 1, 0xffff);
}

uint8_t ReceiveByte(void)
{
    uint8_t data;
    HAL_SPI_Receive(&hspi1, &data, 1, 0xffff);
    return data;
}


// Turn flesh memory device on
void disableFlash(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
}

// Turn flesh memory device off
void enableFlash(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
}

void setAddress(uint32_t addr)
{
    SendByte(addr >> 16);
    SendByte(addr >> 8);
    SendByte(addr);
}

uint8_t readStatusRegister(void)
{
    uint8_t data = 0;
    enableFlash();
    SendByte(0x05);
    data = ReceiveByte();
    disableFlash();
    return data;
}

void waitUntilBusy(void)
{
    while (1)
    {
        if ((readStatusRegister()&0x01)!=0x01) break;
    }
}

void EnableWrite(void)
{
    enableFlash();
    SendByte(0x06);
    disableFlash();
}

void WriteToStatusRegister(uint8_t statusData)
{
    enableFlash();
    SendByte(0x50);
    disableFlash();
    enableFlash();
    SendByte(0x01);
    SendByte(statusData);
    disableFlash();
}

void DisableWrite(void)
{
    enableFlash();
    SendByte(0x04);
    disableFlash();
}

void ClearSector (uint32_t address)
{
    EnableWrite();
    enableFlash();
    SendByte(0x20);
    setAddress(address);
    disableFlash();
    waitUntilBusy();
}

void WriteDataArrayWithAAI(uint32_t address,uint8_t* data,uint32_t len)
{
    EnableWrite();
    enableFlash();
    SendByte(0xAD);
    setAddress(address);
    SendByte(data[0]);
    SendByte(data[1]);
    disableFlash();
    waitUntilBusy();

    for(uint32_t i=1;i<len/2;i++)
    {
        enableFlash();
        SendByte(0xAD);
        SendByte(data[i*2]);
        SendByte(data[i*2+1]);
        disableFlash();
        waitUntilBusy();
    }

    if(len%2==1)
    {
        enableFlash();
        SendByte(0xAD);
        SendByte(data[len-2]);
        SendByte(0x00);
        disableFlash();
        waitUntilBusy();
    }

    DisableWrite();
    waitUntilBusy();
}

void ReadDataArrayFromAddress(uint32_t address,uint8_t* dest,uint32_t count)
{
    enableFlash();
    SendByte(0x03);
    setAddress(address);

    for(uint32_t i=0;i<count;i++)
    {
        dest[i]=ReceiveByte();
    }

    disableFlash();
}

void Clear(void)
{
    EnableWrite();
    enableFlash();
    SendByte(0x60);
    disableFlash();
    waitUntilBusy();
    HAL_Delay(40);
}
