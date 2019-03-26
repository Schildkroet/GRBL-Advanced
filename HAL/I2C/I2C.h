#ifndef I2C_H_INCLUDED
#define I2C_H_INCLUDED


#include <stdint.h>


#define EE_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define EE_LONG_TIMEOUT         ((uint32_t)(30 * EE_FLAG_TIMEOUT))


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    I2C_1 = 0, I2C_2, I2C_3
} I2C_Peripheral_e;


typedef struct {
    uint32_t Speed;
    uint16_t Mode;
    uint16_t Ack;
} I2C_Mode_t;


void I2C_Initialize(I2C_Peripheral_e i2c, I2C_Mode_t *mode);

uint8_t I2C_ReadByte(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr);
uint8_t I2C_WriteByte(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr, uint8_t data);

uint8_t I2C_ReadByteArray(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr, uint8_t *pData, uint16_t Len);
uint8_t I2C_WriteByteArray(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr, uint8_t *pData, uint16_t Len);

void I2C_Scan(I2C_Peripheral_e i2c);


#ifdef __cplusplus
}
#endif


#endif /* I2C_H_INCLUDED */
