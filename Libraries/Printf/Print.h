#ifndef PRINT_H_INCLUDED
#define PRINT_H_INCLUDED


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


void Printf_Init(void);
int Printf(const char *str, ...);
void Printf_Float(float n, uint8_t decimal_places);
int8_t Getc(char *c);
int Putc(const char c);

void Printf_Flush(void);


#ifdef __cplusplus
}
#endif


#endif /* PRINT_H_INCLUDED */
