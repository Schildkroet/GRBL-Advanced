#include "M24C0X.h"
#include "I2C.h"
#include "System32.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_it.h"


#define I2C_SPEED               200000


// Define slave address
#ifndef M24C0X_ADDRESS
    #define M24C0X_ADDRESS      0xA0
#endif

// Define I2C peripheral
#ifndef M24C0X_I2C
    #define M24C0X_I2C          I2C_1
#endif

#define M24C0X_PAGE_SIZE        16

#define WP_DISABLE              0
#define WP_ENABLE               1



static inline void M24C0X_WriteProtection(uint8_t enable);



void M24C0X_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // Initialize write protection pin
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Enable write protection
	GPIO_SetBits(GPIOB, GPIO_Pin_12);

    I2C_Mode_t mode = {I2C_SPEED, I2C_Mode_I2C, I2C_Ack_Enable};
    I2C_Initialize(M24C0X_I2C, &mode);
}


uint8_t M24C0X_ReadByte(uint16_t addr)
{
    uint8_t slave_adr = M24C0X_ADDRESS;

    if(addr > UINT8_MAX)
    {
        // If address is bigger than 255 (8 Bit), upper address bits (A8, A9, A10) are coded into slave address (up to 2KB, bigger EEPROMs use 2 Bytes for addressing)
        slave_adr |=  0x07 & (addr>>8);
    }

    return I2C_ReadByte(M24C0X_I2C, slave_adr, addr);
}


uint8_t M24C0X_WriteByte(uint16_t addr, uint8_t data)
{
    uint8_t slave_adr = M24C0X_ADDRESS;

    if(addr > UINT8_MAX)
    {
        // If address is bigger than 255 (8 Bit), upper address bits (A8, A9, A10) are coded into slave address (up to 2KB, bigger EEPROMs use 2 Bytes for addressing)
        slave_adr |=  0x07 & (addr>>8);
    }

    M24C0X_WriteProtection(WP_DISABLE);
    uint8_t ret = I2C_WriteByte(M24C0X_I2C, slave_adr, addr, data);
    M24C0X_WriteProtection(WP_ENABLE);

    Delay_ms(5);

    return ret;
}


uint8_t M24C0X_ReadByteArray(uint16_t addr, uint8_t *pData, uint16_t len)
{
    uint8_t slave_adr = M24C0X_ADDRESS;

    if(addr > UINT8_MAX)
    {
        // If address is bigger than 255 (8 Bit), upper address bits (A8, A9, A10) are coded into slave address (up to 2KB, bigger EEPROMs use 2 Bytes for addressing)
        slave_adr |=  0x07 & (addr>>8);
    }

    I2C_ReadByteArray(M24C0X_I2C, slave_adr, addr, pData, len);
    return 1;
}


uint8_t M24C0X_WriteByteArray(uint16_t addr, uint8_t *pData, uint16_t len)
{
    uint8_t ret = 0;
    uint8_t bytes2write = len;
    uint16_t remainingBytes = len;
    uint16_t bytesWritten = 0;
    uint8_t timeout = 0;

    uint8_t slave_adr = M24C0X_ADDRESS;


    // Disable write protection
    M24C0X_WriteProtection(WP_DISABLE);

    while(remainingBytes)
    {
        bytes2write = remainingBytes;

        if(addr > UINT8_MAX)
        {
            // If address is bigger than 255 (8 Bit), upper address bits (A8, A9, A10) are coded into slave address (up to 2KB, bigger EEPROMs use 2 Bytes for addressing)
            slave_adr |=  0x07 & (addr>>8);
        }

        // We can write max M24C0X_PAGE_SIZE bytes per write
        if(bytes2write > M24C0X_PAGE_SIZE)
        {
            bytes2write = M24C0X_PAGE_SIZE;
        }

        // Check if page write is aligned
        if(((addr %  M24C0X_PAGE_SIZE) + bytes2write) > M24C0X_PAGE_SIZE)
        {
            bytes2write = M24C0X_PAGE_SIZE - (addr %  M24C0X_PAGE_SIZE);
        }

        // If eeprom is busy (write takes up to 5ms), try again until success or timeout
        while((ret = I2C_WriteByteArray(M24C0X_I2C, slave_adr, addr, &pData[bytesWritten], bytes2write)) && (timeout < 4))
        {
            Delay_ms(2);
            timeout++;
        }

        if(timeout >= 4)
        {
            // Failed to write
            return 1;
        }

        // Reset timeout
        timeout = 0;

        remainingBytes -= bytes2write;
        bytesWritten += bytes2write;
        addr += bytes2write;

        Delay_ms(5);
    }

    // Enable write protection
    M24C0X_WriteProtection(WP_ENABLE);

    return 0;
}


static void M24C0X_WriteProtection(uint8_t enable)
{
	__ASM("nop");
	__ASM("nop");

    if(enable)
    {
        for(uint8_t i = 0; i < 110; i++) __ASM("nop");
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
    }
    else
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    }
    __ASM("nop");
}
