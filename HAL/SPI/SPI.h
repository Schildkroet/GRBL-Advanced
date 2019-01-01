#ifndef SPI_H_INCLUDED
#define SPI_H_INCLUDED


#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_spi.h"


// CS Pin Configuration
// Default SPI1 CS Pins: PA4, PA15(Remap)
#define SPI1_CS_PIN             GPIO_Pin_6
#define SPI1_CS_GPIO_PORT       GPIOB
#define SPI1_CS_GPIO_CLK        RCC_APB2Periph_GPIOB

// Default SPI2 CS Pins: PB12, PB9(AF)
#define SPI2_CS_PIN             GPIO_Pin_12
#define SPI2_CS_GPIO_PORT       GPIOB
#define SPI2_CS_GPIO_CLK        RCC_APB2Periph_GPIOB

// Default SPI3 CS Pins: PA15, PA4(AF)
#define SPI3_CS_PIN             GPIO_Pin_2
#define SPI3_CS_GPIO_PORT       GPIOD
#define SPI3_CS_GPIO_CLK        RCC_AHB1Periph_GPIOD


#define SPI_PRESCALER_2         0x0000
#define SPI_PRESCALER_4         0x0008
#define SPI_PRESCALER_8         0x0010
#define SPI_PRESCALER_16        0x0018
#define SPI_PRESCALER_32        0x0020
#define SPI_PRESCALER_64        0x0028
#define SPI_PRESCALER_128       0x0030
#define SPI_PRESCALER_256       0x0038


#ifdef __cplusplus
 extern "C" {
#endif


typedef enum {
	SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3
} SPI_Mode;


void Spi_Init(SPI_TypeDef *SPIx, SPI_Mode mode);

uint8_t Spi_ReadByte(SPI_TypeDef *SPIx);
uint8_t Spi_WriteByte(SPI_TypeDef *SPIx, uint8_t _data);

void Spi_ReadByteArray(SPI_TypeDef *SPIx, uint8_t *_buffer, uint8_t _len);
void Spi_WriteDataArray(SPI_TypeDef *SPIx, uint8_t *_data, uint8_t _len);

void Spi_SetPrescaler(SPI_TypeDef *SPIx, uint16_t prescaler);
void Spi_ChipSelect(SPI_TypeDef *SPIx, bool select);


#ifdef __cplusplus
}
#endif


#endif /* SPI_H_INCLUDED */
