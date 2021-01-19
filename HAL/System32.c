/*
  System32.c - System Implementation
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
#include "System32.h"


void SysTick_Init(void)
{
    RCC_ClocksTypeDef RCC_Clocks;

    /* SysTick end of count event each 1ms */
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 5));
}


// for 100 MHz STM32F411
#define COUNTS_PER_MICROSECOND  33
void Delay_us(volatile uint32_t us)
{
    volatile uint32_t count = us * COUNTS_PER_MICROSECOND - 2;
    __asm volatile(" mov r0, %[count]  \n\t"
                   "1: subs r0, #1            \n\t"
                   "   bhi 1b                 \n\t"
               :
               : [count] "r" (count)
                       : "r0");
}


void Delay_ms(volatile uint32_t ms)
{
    while(ms--)
        Delay_us(999);
}
