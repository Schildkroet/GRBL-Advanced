/*
  USART.h - USART Header
  Part of STM32F4_HAL

  Copyright (c)	2017 Patrick F.

  STM32F4_HAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  STM32F4_HAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with STM32F4_HAL.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef USART_H_INCLUDED
#define USART_H_INCLUDED

#include <stdbool.h>
#include "stm32f4xx_usart.h"


// Number of USARTs on this device
#define USART_NUM			3

// Numerate available USARTs in ascending order
#define USART1_NUM			0
#define USART2_NUM			1
#define USART6_NUM			2

// Usart used for Printf(...)
#define STDOUT				USART2
#define STDOUT_NUM			USART2_NUM

// Direction definitions
#define USART_DIR_RX		0
#define USART_DIR_TX		1


#ifdef __cplusplus
extern "C" {
#endif


void Usart_Init(USART_TypeDef *usart, uint32_t baud);

void Usart_Put(USART_TypeDef *usart, bool buffered, char c);
void Usart_Write(USART_TypeDef *usart, bool buffered, char *data, uint8_t len);

void Usart_TxInt(USART_TypeDef *usart, bool enable);
void Usart_RxInt(USART_TypeDef *usart, bool enable);


#ifdef __cplusplus
}
#endif


#endif /* USART_H_INCLUDED */
