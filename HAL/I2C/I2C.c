#include "I2C.h"
#include "Print.h"
#include "System32.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_it.h"


static volatile uint32_t EE_Timeout = EE_LONG_TIMEOUT;


void I2C_Initialize(I2C_Peripheral_e i2c, I2C_Mode_t *mode)
{
    I2C_InitTypeDef  I2C_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;


    if(mode != 0)
    {
        /*!< I2C configuration */
        /* sEE_I2C configuration */
        I2C_InitStructure.I2C_Mode = mode->Mode;
        I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
        I2C_InitStructure.I2C_OwnAddress1 = 0x0;
        I2C_InitStructure.I2C_Ack = mode->Ack;
        I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
        I2C_InitStructure.I2C_ClockSpeed = mode->Speed;


        if(i2c == I2C_1)
        {
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

            /* Reset */
            RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);

            /* Release reset signal of sEE_I2C IP */
            RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

            I2C_DeInit(I2C1);

            /*!< GPIO configuration */
            /* Connect PXx to I2C_SCL*/
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);
            /* Connect PXx to I2C_SDA*/
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);

            /*!< Configure sEE_I2C pins: SCL */
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8 ;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
            GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
            GPIO_Init(GPIOB, &GPIO_InitStructure);

            I2C_SoftwareResetCmd(I2C1, ENABLE);
            Delay_ms(1);
            I2C_SoftwareResetCmd(I2C1, DISABLE);

            /* Peripheral Enable */
            I2C_Cmd(I2C1, ENABLE);

            /* Apply configuration after enabling it */
            I2C_Init(I2C1, &I2C_InitStructure);
        }
        else if(i2c == I2C_2)
        {

        }
        else if(i2c == I2C_3)
        {

        }
    }
}


uint8_t I2C_ReadByte(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr)
{
    uint8_t retVal = 0xFF;
    I2C_TypeDef *i2c_dev = 0;


    // Select I2C device
    switch(i2c)
    {
    case I2C_1:
        i2c_dev = I2C1;
        break;

    case I2C_2:
        i2c_dev = I2C2;
        break;

    case I2C_3:
        i2c_dev = I2C3;
        break;
    }

    /*!< While the bus is busy */
    EE_Timeout = EE_LONG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_BUSY))
    {
        if((EE_Timeout--) == 0) return 0xFF;
    }

    /*!< Send START condition */
    I2C_GenerateSTART(i2c_dev, ENABLE);

    /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send EEPROM address for write */
    I2C_Send7bitAddress(i2c_dev, slave_addr, I2C_Direction_Transmitter);

    /*!< Test on EV6 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send the EEPROM's internal address to read from: Only one byte address */
    I2C_SendData(i2c_dev, register_addr);

    /*!< Test on EV8 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_BTF) == RESET)
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send STRAT condition a second time */
    I2C_GenerateSTART(i2c_dev, ENABLE);

    /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send EEPROM address for read */
    I2C_Send7bitAddress(i2c_dev, slave_addr, I2C_Direction_Receiver);

    /* Wait on ADDR flag to be set (ADDR is still not cleared at this level */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_ADDR) == RESET)
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Disable Acknowledgement */
    I2C_AcknowledgeConfig(i2c_dev, DISABLE);

    /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
    (void)i2c_dev->SR2;

    /*!< Send STOP Condition */
    //I2C_GenerateSTOP(i2c_dev, ENABLE);

    /* Wait for the byte to be received */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_RXNE) == RESET)
    {
        if((EE_Timeout--) == 0) return 0xFF;
    }

    /*!< Read the byte received from the EEPROM */
    retVal = I2C_ReceiveData(i2c_dev);

    /*!< Send STOP Condition */
    I2C_GenerateSTOP(i2c_dev, ENABLE);

    /* Wait to make sure that STOP control bit has been cleared */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(i2c_dev->CR1 & I2C_CR1_STOP)
    {
        if((EE_Timeout--) == 0) return 0xFF;
    }

    /*!< Re-Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(i2c_dev, ENABLE);


    return retVal;
}


uint8_t I2C_WriteByte(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr, uint8_t data)
{
    I2C_TypeDef *i2c_dev = 0;


    // Select I2C device
    switch(i2c)
    {
    case I2C_1:
        i2c_dev = I2C1;
        break;

    case I2C_2:
        i2c_dev = I2C2;
        break;

    case I2C_3:
        i2c_dev = I2C3;
        break;
    }


    /*!< While the bus is busy */
    EE_Timeout = EE_LONG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_BUSY))
    {
        if((EE_Timeout--) == 0) return 1;
    }

    /*!< Send START condition */
    I2C_GenerateSTART(i2c_dev, ENABLE);

    /*!< Test on EV5 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 2;
        }
    }

    /*!< Send EEPROM address for write */
    EE_Timeout = EE_FLAG_TIMEOUT;
    I2C_Send7bitAddress(i2c_dev, slave_addr, I2C_Direction_Transmitter);

    /*!< Test on EV6 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 3;
        }
    }

    /*!< Send the EEPROM's internal address to write to : only one byte Address */
    I2C_SendData(i2c_dev, register_addr);

    /*!< Test on EV8 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 4;
        }
    }

    /*!< Send the data */
    I2C_SendData(i2c_dev, data);

    /*!< Test on EV8 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 5;
        }
    }

    /*!<
    return 0; Send STOP Condition */
    I2C_GenerateSTOP(i2c_dev, ENABLE);


    return 0;
}


uint8_t I2C_ReadByteArray(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr, uint8_t *pData, uint16_t Len)
{
    uint8_t retVal = 0;
    I2C_TypeDef *i2c_dev = 0;


    // Select I2C device
    switch(i2c)
    {
    case I2C_1:
        i2c_dev = I2C1;
        break;

    case I2C_2:
        i2c_dev = I2C2;
        break;

    case I2C_3:
        i2c_dev = I2C3;
        break;
    }

    /*!< While the bus is busy */
    EE_Timeout = EE_LONG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_BUSY))
    {
        if((EE_Timeout--) == 0) return 0xFF;
    }

    /*!< Send START condition */
    I2C_GenerateSTART(i2c_dev, ENABLE);

    /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send EEPROM address for write */
    I2C_Send7bitAddress(i2c_dev, slave_addr, I2C_Direction_Transmitter);

    /*!< Test on EV6 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send the EEPROM's internal address to read from: Only one byte address */
    I2C_SendData(i2c_dev, register_addr);

    /*!< Test on EV8 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_BTF) == RESET)
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send STRAT condition a second time */
    I2C_GenerateSTART(i2c_dev, ENABLE);

    /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    /*!< Send EEPROM address for read */
    I2C_Send7bitAddress(i2c_dev, slave_addr, I2C_Direction_Receiver);

    /* Wait on ADDR flag to be set (ADDR is still not cleared at this level */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_ADDR) == RESET)
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 0xFF;
        }
    }

    for(int i = 0; i < Len; i++)
    {
        if(i == Len - 1)
        {
            /*!< Disable Acknowledgement */
            I2C_AcknowledgeConfig(i2c_dev, DISABLE);
        }
        /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
        (void)i2c_dev->SR2;

        /* Wait for the byte to be received */
        EE_Timeout = EE_FLAG_TIMEOUT;
        while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_RXNE) == RESET)
        {
            if((EE_Timeout--) == 0) return 0xFF;
        }

        /*!< Read the byte received from the EEPROM */
        pData[i] = I2C_ReceiveData(i2c_dev);

    }

    /*!< Send STOP Condition */
    I2C_GenerateSTOP(i2c_dev, ENABLE);

    /* Wait to make sure that STOP control bit has been cleared */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(i2c_dev->CR1 & I2C_CR1_STOP)
    {
        if((EE_Timeout--) == 0) return 0xFF;
    }

    /*!< Re-Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(i2c_dev, ENABLE);


    return retVal;
}


uint8_t I2C_WriteByteArray(I2C_Peripheral_e i2c, uint8_t slave_addr, uint16_t register_addr, uint8_t *pData, uint16_t Len)
{
    I2C_TypeDef *i2c_dev = 0;


    // Select I2C device
    switch(i2c)
    {
    case I2C_1:
        i2c_dev = I2C1;
        break;

    case I2C_2:
        i2c_dev = I2C2;
        break;

    case I2C_3:
        i2c_dev = I2C3;
        break;
    }


    /*!< While the bus is busy */
    EE_Timeout = EE_LONG_TIMEOUT;
    while(I2C_GetFlagStatus(i2c_dev, I2C_FLAG_BUSY))
    {
        if((EE_Timeout--) == 0) return 1;
    }

    /*!< Send START condition */
    I2C_GenerateSTART(i2c_dev, ENABLE);

    /*!< Test on EV5 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 2;
        }
    }

    /*!< Send EEPROM address for write */
    EE_Timeout = EE_FLAG_TIMEOUT;
    I2C_Send7bitAddress(i2c_dev, slave_addr, I2C_Direction_Transmitter);

    /*!< Test on EV6 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 3;
        }
    }

    /*!< Send the EEPROM's internal address to write to : only one byte Address */
    I2C_SendData(i2c_dev, register_addr);

    /*!< Test on EV8 and clear it */
    EE_Timeout = EE_FLAG_TIMEOUT;
    while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
    {
        if((EE_Timeout--) == 0)
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);

            return 4;
        }
    }

    for(int i = 0; i < Len; i++)
    {

        /*!< Send the data */
        I2C_SendData(i2c_dev, pData[i]);

        /*!< Test on EV8 and clear it */
        EE_Timeout = EE_FLAG_TIMEOUT;
        while(!I2C_CheckEvent(i2c_dev, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        {
            if((EE_Timeout--) == 0)
            {
                /*!< Send STOP Condition */
                I2C_GenerateSTOP(i2c_dev, ENABLE);

                return 5;
            }
        }

        //DelayMs(6);
    }

    /*!<
    return 0; Send STOP Condition */
    I2C_GenerateSTOP(i2c_dev, ENABLE);

    return 0;
}


void I2C_Scan(I2C_Peripheral_e i2c)
{
	uint8_t adr = 1, cnt = 0;
    I2C_TypeDef *i2c_dev = 0;


    // Select I2C device
    switch(i2c)
    {
    case I2C_1:
        i2c_dev = I2C1;
        break;

    case I2C_2:
        i2c_dev = I2C2;
        break;

    case I2C_3:
        i2c_dev = I2C3;
        break;
    }

	Printf("Scanning I2C...\r\n");
	for(adr = 1; adr < 127; adr++)
    {
		uint8_t ret = I2C_ReadByte(i2c, adr<<1, 0);
		if(ret != 0xFF)
		{
			Printf("Found device at 0x%X\r\n", adr<<1);
			cnt++;
		}
		else
        {
            /*!< Send STOP Condition */
            I2C_GenerateSTOP(i2c_dev, ENABLE);
            Delay_ms(2);
        }
	}
	Printf("Found %d I2C device(s)\r\n", cnt);
}
