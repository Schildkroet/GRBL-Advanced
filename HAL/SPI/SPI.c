#include "SPI.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"


void Spi_Init(SPI_TypeDef *SPIx, SPI_Mode mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;

	switch(mode)
	{
	case SPI_MODE1:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
		break;

	case SPI_MODE2:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
		break;

	case SPI_MODE3:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
		break;

	default:
		// Mode0 default
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
		break;
	}

	if(SPI1 == SPIx) {
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

		// Periph clock enable
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

		GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

		// Configure SPI pins: SCK, MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		// Configure pins: MISO
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		// Reset SPI Interface
		SPI_I2S_DeInit(SPIx);

		// SPI configuration
		SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
		SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
		SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
		SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
		SPI_InitStructure.SPI_CRCPolynomial = 7;
		SPI_Init(SPIx, &SPI_InitStructure);
	}
	else if(SPI2 == SPIx) {
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

		// Periph clock enable
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

		GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

		// Configure SPI pins: SCK, MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		// Configure pins: MISO
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		// Reset SPI Interface
		SPI_I2S_DeInit(SPIx);

		// SPI configuration
		SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
		SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
		SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
		SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
		SPI_InitStructure.SPI_CRCPolynomial = 7;
		SPI_Init(SPIx, &SPI_InitStructure);
	}
	else if(SPI3 == SPIx) {
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

		// Periph clock enable
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

		GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

		// Configure SPI pins: SCK, MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_12;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		// Configure pins: MISO
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		// Reset SPI Interface
		SPI_I2S_DeInit(SPIx);

		// SPI configuration
		SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
		SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
		SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
		SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
		SPI_InitStructure.SPI_CRCPolynomial = 7;
		SPI_Init(SPIx, &SPI_InitStructure);

		// Initialize chip select
		RCC_AHB1PeriphClockCmd(SPI3_CS_GPIO_CLK, ENABLE);

        // Configure CS pin
        GPIO_InitStructure.GPIO_Pin = SPI3_CS_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
		GPIO_Init(SPI3_CS_GPIO_PORT, &GPIO_InitStructure);

        // Deselect chip
        GPIO_SetBits(SPI3_CS_GPIO_PORT, SPI3_CS_PIN);
	}

	SPI_CalculateCRC(SPIx, DISABLE);

	// Enable the SPI
	SPI_Cmd(SPIx, ENABLE);
}


uint8_t Spi_ReadByte(SPI_TypeDef *SPIx)
{
	return Spi_WriteByte(SPIx, 0xFF);
}


uint8_t Spi_WriteByte(SPI_TypeDef *SPIx, uint8_t _data)
{
    uint16_t timeout = 0xFFF;

	// Loop while DR register is not empty
	while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);

	// Send byte through the SPIx peripheral
	SPI_I2S_SendData(SPIx, _data);

	while((SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET) && timeout--);

	// Return the byte read from the SPI bus
	return (uint8_t)SPI_I2S_ReceiveData(SPIx);
}


void Spi_ReadByteArray(SPI_TypeDef *SPIx, uint8_t *_buffer, uint8_t _len)
{
	uint8_t i = 0;
	uint16_t timeout = 0xFFF;

	for(i = 0; i < _len; ++i)
    {
		// Loop while DR register is not empty
		while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);

		// Send byte through the SPIx peripheral
		SPI_I2S_SendData(SPIx, 0xFF);

		while((SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET) && timeout--);

		_buffer[i] = (uint8_t)SPI_I2S_ReceiveData(SPIx);
	}
}

void Spi_WriteDataArray(SPI_TypeDef *SPIx, uint8_t *_data, uint8_t _len)
{
	uint8_t i = 0;

	for(i = 0; i < _len; ++i)
    {
		// Loop while DR register is not empty
		while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);

		// Send byte through the SPIx peripheral
		SPI_I2S_SendData(SPIx, _data[i]);
	}
}


void Spi_SetPrescaler(SPI_TypeDef *SPIx, uint16_t prescaler)
{
    SPI_Cmd(SPIx, DISABLE);

    // Read CR1 and clear baud control
    uint16_t tmpreg = SPIx->CR1 & 0xFFC7;

    tmpreg |= prescaler;

    SPIx->CR1 = tmpreg;

    SPI_Cmd(SPIx, ENABLE);
}


void Spi_ChipSelect(SPI_TypeDef *SPIx, bool select)
{
    if(select)
    {
        if(SPIx == SPI1)
        {
            GPIO_ResetBits(SPI1_CS_GPIO_PORT, SPI1_CS_PIN);
        }
        else if(SPIx == SPI2)
        {
            GPIO_ResetBits(SPI2_CS_GPIO_PORT, SPI2_CS_PIN);
        }
        else if(SPIx == SPI3)
        {
            GPIO_ResetBits(SPI3_CS_GPIO_PORT, SPI3_CS_PIN);
        }
    }
    else
    {
        if(SPIx == SPI1)
        {
            GPIO_SetBits(SPI1_CS_GPIO_PORT, SPI1_CS_PIN);
        }
        else if(SPIx == SPI2)
        {
            GPIO_SetBits(SPI2_CS_GPIO_PORT, SPI2_CS_PIN);
        }
        else if(SPIx == SPI3)
        {
            GPIO_SetBits(SPI3_CS_GPIO_PORT, SPI3_CS_PIN);
        }
    }
}
