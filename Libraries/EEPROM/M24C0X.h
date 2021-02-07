#ifndef M24C0X_H_INCLUDED
#define M24C0X_H_INCLUDED


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


void M24C0X_Init(void);

uint8_t M24C0X_ReadByte(uint16_t addr);
uint8_t M24C0X_WriteByte(uint16_t addr, uint8_t data);

uint8_t M24C0X_ReadByteArray(uint16_t addr, uint8_t *pData, uint16_t len);
uint8_t M24C0X_WriteByteArray(uint16_t addr, uint8_t *pData, uint16_t len);


#ifdef __cplusplus
}
#endif


#endif /* M24C0X_H_INCLUDED */
