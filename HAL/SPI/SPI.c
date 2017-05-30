#include "SPI.h"


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

		GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_SPI3);

		// Configure SPI pins: SCK, MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		// Configure pins: MISO
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
		GPIO_Init(GPIOC, &GPIO_InitStructure);

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
	// Loop while DR register is not empty
	while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);

	// Send byte through the SPIx peripheral
	SPI_I2S_SendData(SPIx, _data);

	while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);

	// Return the byte read from the SPI bus
	return (uint8_t)SPI_I2S_ReceiveData(SPIx);
}

