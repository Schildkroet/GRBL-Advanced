/*
  FIFO_USART.h - FIFO USART Header
  Part of STM32F4_HAL

  Copyright (c)	2016-2017 Patrick F.

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
#ifndef FIFO_H_INCLUDED
#define FIFO_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "Usart.h"


/* Queue structure */
#define QUEUE_ELEMENTS 		320
#define QUEUE_SIZE 			(QUEUE_ELEMENTS + 1)


#ifdef __cplusplus
extern "C" {
#endif


void FifoUsart_Init(void);
int8_t FifoUsart_Insert(uint8_t usart, uint8_t direction, char ch);
int8_t FifoUsart_Get(uint8_t usart, uint8_t direction, char *ch);
uint32_t FifoUsart_Available(uint8_t usart);


#ifdef __cplusplus
}
#endif


#endif /* FIFO_H_INCLUDED */
