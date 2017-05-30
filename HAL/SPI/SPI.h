#ifndef SPI_H_INCLUDED
#define SPI_H_INCLUDED


#include <stdint.h>
#include "stm32f4xx_conf.h"


typedef enum {
	SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3
} SPI_Mode;


#ifdef __cplusplus
 extern "C" {
#endif


void Spi_Init(SPI_TypeDef *SPIx, SPI_Mode mode);

uint8_t Spi_ReadByte(SPI_TypeDef *SPIx);
uint8_t Spi_WriteByte(SPI_TypeDef *SPIx, uint8_t _data);


#ifdef __cplusplus
}
#endif


#endif /* SPI_H_INCLUDED */
