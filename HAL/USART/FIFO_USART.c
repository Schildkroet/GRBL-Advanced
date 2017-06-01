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
 * calls to QueuePut fail.
 */
#include <string.h>
#include "FIFO_USART.h"
#include "debug.h"


static char FifoQueue[USART_NUM][2][QUEUE_SIZE];
static uint8_t QueueIn[2][USART_NUM], QueueOut[2][USART_NUM];


void FifoUsart_Init(void)
{
    memset(QueueIn, 0, sizeof(QueueIn));
    memset(QueueOut, 0, sizeof(QueueOut));
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

    return 0; // No errors
}
