/*
  FIFO_USART.c - FIFO USART Implementation
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

/* Very simple queue
 * These are FIFO queues which discard the new data when full.
 *
 * Queue is empty when in == out.
 * If in != out, then
 *  - items are placed into in before incrementing in
 *  - items are removed from out before incrementing out
 * Queue is full when in == (out-1 + QUEUE_SIZE) % QUEUE_SIZE;
 *
 * The queue will hold QUEUE_ELEMENTS number of items before the
 * calls to FifoUsart_Insert fail.
 */
#include <string.h>
#include "FIFO_USART.h"
#include "debug.h"


static char FifoQueue[USART_NUM][2][QUEUE_SIZE];
static uint8_t QueueIn[2][USART_NUM], QueueOut[2][USART_NUM];
static uint32_t Count[USART_NUM] = {0};


void FifoUsart_Init(void)
{
    memset(QueueIn, 0, sizeof(QueueIn));
    memset(QueueOut, 0, sizeof(QueueOut));
    memset(Count, 0, sizeof(Count));
}


int8_t FifoUsart_Insert(uint8_t usart, uint8_t direction, char ch)
{
	if(usart >= USART_NUM) {
		d_printf("ERROR: Wrong USART %d\n", usart);

		return -1;
	}
	if(direction > 1) {
		d_printf("ERROR: USART direction out of range\n");

		return -1;
	}

    if(QueueIn[direction][usart] == ((QueueOut[direction][usart] - 1 + QUEUE_SIZE) % QUEUE_SIZE))
    {
        return -1; // Queue Full
    }

    FifoQueue[usart][direction][QueueIn[direction][usart]] = ch;

    QueueIn[direction][usart] = (QueueIn[direction][usart] + 1) % QUEUE_SIZE;

    Count[usart]++;

    return 0; // No errors
}


int8_t FifoUsart_Get(uint8_t usart, uint8_t direction, char *ch)
{
	if(usart >= USART_NUM) {
		d_printf("ERROR: Wrong USART %d\n", usart);

		return -1;
	}
	if(direction > 1) {
		d_printf("ERROR: USART direction out of range\n");

		return -1;
	}

    if(QueueIn[direction][usart] == QueueOut[direction][usart])
    {
        return -1; /* Queue Empty - nothing to get*/
    }

    *ch = FifoQueue[usart][direction][QueueOut[direction][usart]];

    QueueOut[direction][usart] = (QueueOut[direction][usart] + 1) % QUEUE_SIZE;

    Count[usart]--;

    return 0; // No errors
}


uint32_t FifoUsart_Available(uint8_t usart)
{
    if(usart >= USART_NUM) {
		d_printf("ERROR: Wrong USART %d\n", usart);

		return 0xFFFFFFFF;
	}

    return (QUEUE_ELEMENTS - Count[usart]);
}
