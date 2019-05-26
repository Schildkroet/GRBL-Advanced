/*
  W5500.c - Driver for WIZnet W5500.

  Copyright (c) 2018 Patrick F.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "W5500.h"
#include "SPI.h"
#include "System32.h"


#ifndef SPI_W5500
    #define SPI_W5500           SPI3
#endif

#define RESET_BIT               7


static inline void Write(uint16_t _addr, uint8_t _cb, uint8_t _data);
static inline uint16_t WriteArray(uint16_t addr, uint8_t _cb, const uint8_t *buf, uint16_t len);
static inline uint8_t Read(uint16_t addr, uint8_t _cb);
static inline uint16_t ReadArray(uint16_t addr, uint8_t _cb, uint8_t *buf, uint16_t len);


static inline uint8_t ReadSn(SOCKET _s, uint16_t _addr);
static inline void WriteSn(SOCKET _s, uint16_t _addr, uint8_t _data);
static inline uint16_t ReadSnArray(SOCKET _s, uint16_t _addr, uint8_t *_buf, uint16_t len);
static inline uint16_t WriteSnArray(SOCKET _s, uint16_t _addr, uint8_t *_buf, uint16_t len);


void W5500_Init(void)
{
    Spi_Init(SPI_W5500, SPI_MODE0);

    // Set clock to 21 Mhz (W5500 should support up to about 80 Mhz)
    Spi_SetPrescaler(SPI_W5500, SPI_PRESCALER_2);

    GPIO_ResetBits(GPIOA, GPIO_Pin_15);
    Delay_ms(40);
    GPIO_SetBits(GPIOA, GPIO_Pin_15);
    Delay_ms(40);

    W5500_SoftReset();
    Delay_ms(40);

    for(uint8_t i = 0; i < MAX_SOCK_NUM; i++)
    {
        uint8_t cntl_byte = (0x0C + (i<<5));

        Write(0x1E, cntl_byte, 4);     //0x1E - Sn_RXBUF_SIZE - 4k
        Write(0x1F, cntl_byte, 2);     //0x1F - Sn_TXBUF_SIZE - 2k
    }
}


void W5500_SoftReset(void)
{
    uint8_t cnt = 0;

	// Write reset bit
	W5500_WRITE_GP_REG8(REG8_MR, 1<<RESET_BIT);
	Delay_ms(1);

	// Wait until reset is complete or timeout
	while(cnt++ < 32)
    {
		if(W5500_READ_GP_REG8(REG8_MR) == 0)
        {
            return;
        }

		Delay_ms(1);
	}
}


void W5500_ReadData(SOCKET s, volatile uint16_t src, volatile uint8_t *dst, uint16_t len)
{
    uint8_t cntl_byte = (0x18 + (s<<5));

    ReadArray((uint16_t)src , cntl_byte, (uint8_t*)dst, len);
}


void W5500_SendDataProcessing(SOCKET s, const uint8_t *data, uint16_t len)
{
    W5500_SendDataProcessingOffset(s, 0, data, len);
}


void W5500_SendDataProcessingOffset(SOCKET s, uint16_t data_offset, const uint8_t *data, uint16_t len)
{
    uint16_t ptr = W5500_READ_SOCK_REG16(s, REG16_SnTX_WR);
    uint8_t cntl_byte = (0x14+(s<<5));

    ptr += data_offset;
    WriteArray(ptr, cntl_byte, data, len);

    ptr += len;
    W5500_WRITE_SOCK_REG16(s, REG16_SnTX_WR, ptr);
}


void W5500_RecvDataProcessing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek)
{
    uint16_t ptr;
    ptr = W5500_READ_SOCK_REG16(s, REG16_SnRX_RD);

    W5500_ReadData(s, ptr, data, len);

    if(!peek)
    {
        ptr += len;
        W5500_WRITE_SOCK_REG16(s, REG16_SnRX_RD, ptr);
    }
}


void W5500_GetGatewayIp(uint8_t *_addr)
{
    W5500_READ_GP_REGN(REGN_GAR_4, _addr, 4);
}


void W5500_SetGatewayIp(uint8_t *_addr)
{
    W5500_WRITE_GP_REGN(REGN_GAR_4, _addr, 4);
}


void W5500_GetSubnetMask(uint8_t *_addr)
{
    W5500_READ_GP_REGN(REGN_SUBR_4, _addr, 4);
}


void W5500_SetSubnetMask(uint8_t *_addr)
{
    W5500_WRITE_GP_REGN(REGN_SUBR_4, _addr, 4);
}


void W5500_GetMACAddress(uint8_t *_addr)
{
    W5500_READ_GP_REGN(REGN_SHAR_6, _addr, 6);
}


void W5500_SetMACAddress(uint8_t *_addr)
{
    W5500_WRITE_GP_REGN(REGN_SHAR_6, _addr, 6);
}


void W5500_GetIPAddress(uint8_t *_addr)
{
    W5500_READ_GP_REGN(REGN_SIPR_4, _addr, 4);
}


void W5500_SetIPAddress(uint8_t *_addr)
{
    W5500_WRITE_GP_REGN(REGN_SIPR_4, _addr, 4);
}


void W5500_SetRetransmissionTime(uint16_t _timeout)
{
    W5500_WRITE_GP_REG16(REG16_RTR, _timeout);
}


void W5500_SetRetransmissionCount(uint8_t _retry)
{
    W5500_WRITE_GP_REG8(REG8_RCR, _retry);
}


void W5500_ExecCmdSn(SOCKET s, SockCMD_e _cmd)
{
    // Send command to socket
    W5500_WRITE_SOCK_REG8(s, REG8_SnCR, _cmd);

    // Wait for command to complete
    while(W5500_READ_SOCK_REG8(s, REG8_SnCR));
}


uint16_t W5500_GetTXFreeSize(SOCKET s)
{
    uint16_t val = 0, val1 = 0;

    do
    {
        val1 = W5500_READ_SOCK_REG16(s, REG16_SnTX_FSR);
        if(val1 != 0)
        {
            val = W5500_READ_SOCK_REG16(s, REG16_SnTX_FSR);
        }
    } while(val != val1);

    return val;
}


uint16_t W5500_GetRXReceivedSize(SOCKET s)
{
    uint16_t val = 0, val1 = 0;

    do
    {
        val1 = W5500_READ_SOCK_REG16(s, REG16_SnRX_RSR);
        if(val1 != 0)
        {
            val = W5500_READ_SOCK_REG16(s, REG16_SnRX_RSR);
        }
    } while (val != val1);

    return val;
}


void W5500_SetPHYCFGR(uint8_t _val)
{
    W5500_WRITE_GP_REG8(REG8_PHYCFGR, _val);
}


uint8_t W5500_GetPHYCFGR(void)
{
    return Read(0x002E, 0x00);
}


static inline void Write(uint16_t _addr, uint8_t _cb, uint8_t _data)
{
    Spi_ChipSelect(SPI_W5500, true);

    Spi_WriteByte(SPI_W5500, _addr >> 8);
    Spi_WriteByte(SPI_W5500, _addr & 0xFF);
    Spi_WriteByte(SPI_W5500, _cb);
    Spi_WriteByte(SPI_W5500, _data);

    Spi_ChipSelect(SPI_W5500, false);
    __ASM("nop");
    __ASM("nop");
}


static inline uint16_t WriteArray(uint16_t _addr, uint8_t _cb, const uint8_t *_buf, uint16_t _len)
{
    Spi_ChipSelect(SPI_W5500, true);

    Spi_WriteByte(SPI_W5500, _addr >> 8);
    Spi_WriteByte(SPI_W5500, _addr & 0xFF);
    Spi_WriteByte(SPI_W5500, _cb);

    for(uint16_t i = 0; i < _len; i++)
    {
        Spi_WriteByte(SPI_W5500, _buf[i]);
    }

    Spi_ChipSelect(SPI_W5500, false);
    __ASM("nop");
    __ASM("nop");

    return _len;
}


static inline uint8_t Read(uint16_t _addr, uint8_t _cb)
{
    Spi_ChipSelect(SPI_W5500, true);

    Spi_WriteByte(SPI_W5500, _addr >> 8);
    Spi_WriteByte(SPI_W5500, _addr & 0xFF);
    Spi_WriteByte(SPI_W5500, _cb);

    uint8_t _data = Spi_ReadByte(SPI_W5500);

    Spi_ChipSelect(SPI_W5500, false);
    __ASM("nop");
    __ASM("nop");

    return _data;
}


static inline uint16_t ReadArray(uint16_t _addr, uint8_t _cb, uint8_t *_buf, uint16_t _len)
{
    Spi_ChipSelect(SPI_W5500, true);

    Spi_WriteByte(SPI_W5500, _addr >> 8);
    Spi_WriteByte(SPI_W5500, _addr & 0xFF);
    Spi_WriteByte(SPI_W5500, _cb);

    for(uint16_t i = 0; i < _len; i++)
    {
        _buf[i] = Spi_ReadByte(SPI_W5500);
    }

    Spi_ChipSelect(SPI_W5500, false);
    __ASM("nop");
    __ASM("nop");

    return _len;
}


static inline uint8_t ReadSn(SOCKET _s, uint16_t _addr)
{
    uint8_t cntl_byte = (_s<<5) + 0x08;

    return Read(_addr, cntl_byte);
}


static inline void WriteSn(SOCKET _s, uint16_t _addr, uint8_t _data)
{
    uint8_t cntl_byte = (_s<<5) + 0x0C;

    Write(_addr, cntl_byte, _data);
}


static inline uint16_t ReadSnArray(SOCKET _s, uint16_t _addr, uint8_t *_buf, uint16_t _len)
{
    uint8_t cntl_byte = (_s<<5) + 0x08;

    return ReadArray(_addr, cntl_byte, _buf, _len);
}


static inline uint16_t WriteSnArray(SOCKET _s, uint16_t _addr, uint8_t *_buf, uint16_t _len)
{
    uint8_t cntl_byte = (_s<<5) + 0x0C;

    return WriteArray(_addr, cntl_byte, _buf, _len);
}


// W5500 Registers
// ---------------
void W5500_WRITE_GP_REG8(uint16_t address, uint8_t _data)
{
    Write(address, 0x04, _data);
}


uint8_t W5500_READ_GP_REG8(uint16_t address)
{
    return Read(address, 0x00);
}


void W5500_WRITE_GP_REG16(uint16_t address, uint16_t _data)
{
    Write(address,  0x04, _data >> 8);
    Write(address + 1, 0x04, _data & 0xFF);
}


uint16_t W5500_READ_GP_REG16(uint16_t address)
{
    uint16_t res = Read(address, 0x00);
    res = (res << 8) + Read(address + 1, 0x00);

    return res;
}


uint16_t W5500_WRITE_GP_REGN(uint16_t address, uint8_t *_buff, uint16_t size)
{
    return WriteArray(address, 0x04, _buff, size);
}


uint16_t W5500_READ_GP_REGN(uint16_t address, uint8_t *_buff, uint16_t size)
{
    return ReadArray(address, 0x00, _buff, size);
}


// W5500 Socket registers
// ----------------------

void W5500_WRITE_SOCK_REG8(SOCKET _s, uint16_t address, uint8_t _data)
{
    WriteSn(_s, address, _data);
}


uint8_t W5500_READ_SOCK_REG8(SOCKET _s, uint16_t address)
{
    return ReadSn(_s, address);
}


void W5500_WRITE_SOCK_REG16(SOCKET _s, uint16_t address, uint16_t _data)
{
    WriteSn(_s, address,   _data >> 8);                      \
    WriteSn(_s, address+1, _data & 0xFF);
}


uint16_t W5500_READ_SOCK_REG16(SOCKET _s, uint16_t address)
{
    uint16_t res = ReadSn(_s, address);
    uint16_t res2 = ReadSn(_s, address + 1);

    res = res << 8;
    res2 = res2 & 0xFF;
    res = res | res2;

    return res;
}


uint16_t W5500_WRITE_SOCK_REGN(SOCKET _s, uint16_t address, uint8_t *_buff, uint16_t size)
{
    return WriteSnArray(_s, address, _buff, size);
}


uint16_t W5500_READ_SOCK_REGN(SOCKET _s, uint16_t address, uint8_t *_buff, uint16_t size)
{
    return ReadSnArray(_s, address, _buff, size);
}
