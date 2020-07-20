/*
  ComIf.c - Communication Interface

  Copyright (c) 2018-2019 Patrick F.

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
#include "ComIf.h"
#include "ServerTCP.h"
#include "Platform.h"
#include "Print.h"
#include "Usart.h"
#include <string.h>


// Buffer size of interface
#ifndef COMIF_BUFFER_SIZE
    #define COMIF_BUFFER_SIZE       512
#endif

#define MAX_READ_SIZE               64


static inline uint16_t BufferFreeBytes(void);
static inline void BufferWriteByte(uint8_t data);
static inline uint8_t BufferReadByte(void);


static uint8_t RxBuffer[COMIF_BUFFER_SIZE] = {0};
static uint16_t Head = 0, Tail = 0, Size = 0;

static uint8_t Socket = 0;
static uint8_t Interface = IF_USB;


void ComIf_Init(uint8_t interface, uint8_t sock)
{
    Head = 0, Tail = 0, Size = 0;
    memset(RxBuffer, 0, COMIF_BUFFER_SIZE);
    Socket = sock;
    Interface = interface;
}


void ComIf_DeInit(void)
{
    ComIf_Init(Interface, 0);
}


uint8_t ComIf_Send(uint8_t *data, uint16_t len)
{
    if(Interface == IF_ETH)
    {
        return ServerTCP_Send(Socket, data, len);
    }
    else
    {
        Usart_Write(STDOUT, false, (char*)data, len);
    }

    return 0;
}


uint16_t ComIf_Receive(uint8_t *data, uint16_t len)
{
    // Check if data available
    uint16_t rec = ComIf_DataAvailable();
    if(rec)
    {
        if(len > rec)
        {
            len = rec;
        }

        for(uint16_t i = 0; i < len; i++)
        {
            data[i] = BufferReadByte();
        }

        return len;
    }

    // No data available
    return 0;
}


uint16_t ComIf_DataAvailable(void)
{
    return (COMIF_BUFFER_SIZE - BufferFreeBytes());
}


void ComIf_Update(void)
{
    uint8_t ret = 0;

    if(Interface == IF_ETH)
    {
        ret = ServerTCP_DataAvailable(Socket);
    }
    else
    {
        // Assume, that there is always data available on serial
        ret = 1;
    }

    if(ret && (MAX_READ_SIZE <= BufferFreeBytes()))
    {
        uint8_t buf[MAX_READ_SIZE];

        // Read max 64 bytes
        if(ret > MAX_READ_SIZE)
        {
            ret = MAX_READ_SIZE;
        }

        if(Interface == IF_ETH)
        {
            // Read max 64 bytes from TCP socket and write it into buffer
            uint8_t read = ServerTCP_Receive(Socket, buf, ret);

            for(uint8_t i = 0; i < read; i++)
            {
                BufferWriteByte(buf[i]);
            }
        }
        else
        {
            // Read max 64 bytes from usart buffer
            for(uint16_t read = 0; read < MAX_READ_SIZE; read++)
            {
                char c;
                if(Getc(&c) == 0)
                {
                    BufferWriteByte((uint8_t)c);
                }
                else
                {
                    break;
                }
            }
        }
    }
}


static inline void BufferWriteByte(uint8_t data)
{
    if(Size > (COMIF_BUFFER_SIZE-1))
    {
        // Queue full
        return;
    }

    RxBuffer[Head] = data;
    Head = (Head + 1) % COMIF_BUFFER_SIZE;
    Size++;
}


static inline uint8_t BufferReadByte(void)
{
    uint8_t ret = 0;

    if(Size)
    {
        ret = RxBuffer[Tail];
        Tail = (Tail + 1) % COMIF_BUFFER_SIZE;
        Size--;
    }

    return ret;
}


static inline uint16_t BufferFreeBytes(void)
{
    return (COMIF_BUFFER_SIZE - Size);
}
