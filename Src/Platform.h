#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED


#include "stm32f4xx_conf.h"


//---- I2C ----//
#define EEPROM_I2C			I2C1


//---- SPI ----//
#define SPI_W5500			SPI3


//---- USART ----//
// Number of used USARTs on this device
//#define USART_NUM			2

// Numerate available USARTs in ascending order
//#define USART1_NUM			0
//#define USART2_NUM			1
//#define USART6_NUM			2

// USART used for Printf(...)
#define STDOUT				USART2
#define STDOUT_NUM			USART2_NUM
#define STDOUT_BAUD			230400


//---- Defines ----//

// Direction definitions
#define FIFO_DIR_RX		    0
#define FIFO_DIR_TX		    1


// Communication interface
// Comment out to use serial interface
//#define ETH_IF

#define ETH_SOCK            0
#define ETH_PORT            30501


#endif /* PLATFORM_H_INCLUDED */
