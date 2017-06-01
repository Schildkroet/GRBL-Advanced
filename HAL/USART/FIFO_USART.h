#ifndef FIFO_H_INCLUDED
#define FIFO_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "USART.h"


/* Queue structure */
#define QUEUE_ELEMENTS 		256
#define QUEUE_SIZE 			(QUEUE_ELEMENTS + 1)


#ifdef __cplusplus
extern "C" {
#endif


void FifoUsart_Init(void);
int8_t FifoUsart_Insert(uint8_t usart, uint8_t direction, char ch);
int8_t FifoUsart_Get(uint8_t usart, uint8_t direction, char *ch);


#ifdef __cplusplus
}
#endif


#endif /* FIFO_H_INCLUDED */
