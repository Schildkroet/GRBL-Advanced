/*
  System32.h - System Header
  Part of STM32F4_HAL

  Copyright (c) 2017 Patrick F.

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
#ifndef SYSTEM32_H_INCLUDED
#define SYSTEM32_H_INCLUDED

#include "stm32f4xx_conf.h"
#include "stm32f4xx_it.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} Time_t;

typedef struct
{
    uint16_t Year;
    uint8_t Month;
    uint8_t Day;
} Date_t;



void SysTick_Init(void);
void Delay_us(volatile uint32_t us);
void Delay_ms(volatile uint32_t ms);


#ifdef __cplusplus
}
#endif


#endif /* SYSTEM32_H_INCLUDED */
