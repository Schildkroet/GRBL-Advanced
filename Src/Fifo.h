#ifndef FIFO_H_INCLUDED
#define FIFO_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#define DIR_RX		0
#define DIR_TX		1


void Fifo_Init(void);
int8_t FifoUSART_Insert(uint8_t usart, uint8_t direction, char ch);
int8_t FifoUSART_Get(uint8_t usart, uint8_t direction, char *ch);


#ifdef __cplusplus
}
#endif


#endif /* FIFO_H_INCLUDED */
