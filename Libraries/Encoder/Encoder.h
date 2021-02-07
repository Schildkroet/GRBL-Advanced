#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED


#include <stdint.h>


#define PULSES_PER_REV      360


#ifdef __cplusplus
extern "C" {
#endif


void Encoder_Init(void);
void Encoder_Reset(void);

uint32_t Encoder_GetValue(void);
void Encoder_SetValue(uint32_t val);

void Encoder_OvfISR(void);


#ifdef __cplusplus
}
#endif


#endif /* ENCODER_H_INCLUDED */
