#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


void Encoder_Init(uint16_t ppr);
void Encoder_Reset(void);

void Encoder_SetPulsesPerRev(uint16_t ppr);

uint32_t Encoder_GetValue(void);
void Encoder_SetValue(uint32_t val);

void Encoder_OvfISR(void);


#ifdef __cplusplus
}
#endif


#endif /* ENCODER_H_INCLUDED */
