/*
  GrIP.c - GRBL over IP

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
#include "GrIP.h"
#include "CRC.h"
#include "util2.h"
#include "ComIf.h"
#include "Print.h"
#include <stdio.h>
#include <string.h>


// GrIP states
#define GRIP_IDLE               0
#define GRIP_RX_HEADER          1
#define GRIP_RX_DATA            2

// Magic byte - Marks start of transmission
#define MAGIC                   0x55

// Size of header packet
#define GRIP_HEADER_SIZE        (sizeof(GrIP_PacketHeader_t))


static uint8_t CheckHeader(GrIP_PacketHeader_t *paket);


static GrIP_PacketHeader_t TX_Header;

// Transmit Buffer
static uint8_t TX_Buffer[GRIP_BUFFER_SIZE + GRIP_HEADER_SIZE];
// Receive Data Array
static RX_Packet_t RX_Buff[GRIP_RX_NUM] = {0};

static uint8_t GrIP_Status = GRIP_IDLE;
static uint8_t GrIP_Response = RESPONSE_OK;
static uint8_t GrIP_idx = 0;


void GrIP_Init(void)
{
    // Initialize to default values
    GrIP_Status = GRIP_IDLE;
    GrIP_idx = 0;

    memset(&TX_Header, 0, GRIP_HEADER_SIZE);

    memset(TX_Buffer, 0, GRIP_BUFFER_SIZE + GRIP_HEADER_SIZE);
    memset(RX_Buff, 0, sizeof(RX_Buff));

    // Init generic interface
    ComIf_Init(IF_ETH, 0);
    // Init CRC module
    CRC_Init();
}


uint8_t GrIP_Transmit(uint8_t MsgType, uint8_t ReturnCode, Pdu_t *data)
{
    // Prepare header
    TX_Header.Version = GRIP_VERSION;
    TX_Header.MsgType = MsgType;
    TX_Header.ReturnCode = ReturnCode;

    if(data)
    {
        // Convert length to network order
        TX_Header.Length = htons(data->Length);

        // Check if data fits into transmit buffer
        if(data->Length > GRIP_BUFFER_SIZE)
        {
            return RET_NOK;
        }
        else if(data->Length > 0)
        {
            // Calculate CRC of data
            TX_Header.CRC8 = CRC_CalculateCRC8(data->Data, data->Length);
        }
        else
        {
            // No data, no CRC
            TX_Header.CRC8 = 0;
        }

        // Prepare transmit buffer
        TX_Buffer[0] = MAGIC;
        memcpy(&TX_Buffer[1], &TX_Header, GRIP_HEADER_SIZE);
        memcpy(&TX_Buffer[1] + GRIP_HEADER_SIZE, data->Data, data->Length);

        // Transmit paket
        ComIf_Send(TX_Buffer, data->Length + GRIP_HEADER_SIZE + 1);

        // Check if we are expecting a response
        GrIP_Response = RESPONSE_OK;
        if(MsgType == MSG_DATA)
        {
            GrIP_Response = RESPONSE_WAIT;
        }

        return RET_OK;
    }
    else    // Response
    {
        // No data available -> Response
        // No data to transmit, only header
        TX_Header.Length = 0;

        memcpy(TX_Buffer, &TX_Header, GRIP_HEADER_SIZE);

        // Transmit paket
        ComIf_Send(TX_Buffer, GRIP_HEADER_SIZE);

        return RET_OK;
    }

    // Clear memory
    memset(&TX_Header, 0, GRIP_HEADER_SIZE);
    memset(TX_Buffer, 0, GRIP_BUFFER_SIZE);

    return RET_NOK;
}


uint8_t GrIP_Receive(RX_Packet_t *pData)
{
    if(pData)
    {
        for(uint8_t i = 0; i < GRIP_RX_NUM; i++)
        {
            if(RX_Buff[i].isValid)
            {
                memcpy(pData, &RX_Buff[i], sizeof(RX_Packet_t));

                // Clear rx slot
                memset(&RX_Buff[i], 0, sizeof(RX_Packet_t));

                return 1;
            }
        }
    }

    // No data available
    return 0;
}


uint8_t GrIP_ResponseStatus(void)
{
    return GrIP_Response;
}


void GrIP_Update(void)
{
    switch(GrIP_Status)
    {
    case GRIP_IDLE:
        // Check if data is available
        if(ComIf_DataAvailable())
        {
            uint8_t magic = 0;

            // Read Magic byte
            ComIf_Receive(&magic, 1);

            if(magic == MAGIC)
            {
                // Received valid packet
                GrIP_Status = GRIP_RX_HEADER;
            }
        }
        break;

    case GRIP_RX_HEADER:
        // Check if header is available
        if(ComIf_DataAvailable() > (GRIP_HEADER_SIZE-1))
        {
            uint8_t head_buff[GRIP_HEADER_SIZE] = {0};

            // Get header
            ComIf_Receive(head_buff, GRIP_HEADER_SIZE);

            // Fill struct
            memcpy(&RX_Buff[GrIP_idx].RX_Header, head_buff, GRIP_HEADER_SIZE);

            // Convert length to host order
            RX_Buff[GrIP_idx].RX_Header.Length = ntohs(RX_Buff[GrIP_idx].RX_Header.Length);

            // Check if header is valid
            uint8_t ret = CheckHeader(&RX_Buff[GrIP_idx].RX_Header);
            if(ret != RET_OK)
            {
                // Header is invalid
                GrIP_Status = GRIP_IDLE;
                //Printf("Wrong header: %d\n", ret);
                break;
            }
            if(RX_Buff[GrIP_idx].RX_Header.Length > GRIP_BUFFER_SIZE)
            {
                // Payload too big
                GrIP_Status = GRIP_IDLE;
                //Printf("Payload exceeds limit: %d\n", RX_Buff[GrIP_idx].RX_Header.Length);
                break;
            }

            // If payload is available
            if(RX_Buff[GrIP_idx].RX_Header.Length)
            {
                // Payload is available
                GrIP_Status = GRIP_RX_DATA;
            }
            else
            {
                // No payload
                GrIP_Status = GRIP_IDLE;

                RX_Buff[GrIP_idx].isValid = 1;

                if(GrIP_idx < (GRIP_RX_NUM-1))
                {
                    GrIP_idx++;
                }
                else
                {
                    GrIP_idx = 0;
                }
            }
        }
        break;

    case GRIP_RX_DATA:
        // Check if entire payload is available
        if(ComIf_DataAvailable() > (RX_Buff[GrIP_idx].RX_Header.Length-1))
        {
            // Get payload
            //GenIf_Receive(RX_Buffer, RX_Header.Length);
            ComIf_Receive(RX_Buff[GrIP_idx].Data, RX_Buff[GrIP_idx].RX_Header.Length);
            if(RX_Buff[GrIP_idx].RX_Header.CRC8 == CRC_CalculateCRC8(RX_Buff[GrIP_idx].Data, RX_Buff[GrIP_idx].RX_Header.Length))
            {
                RX_Buff[GrIP_idx].isValid = 1;

                if(GrIP_idx < (GRIP_RX_NUM-1))
                {
                    GrIP_idx++;
                }
                else
                {
                    GrIP_idx = 0;
                }
            }
            GrIP_Status = GRIP_IDLE;
        }
        break;
    }

    // Check for new data
    ComIf_Update();
}


static uint8_t CheckHeader(GrIP_PacketHeader_t *paket)
{
    if(paket->Version != GRIP_VERSION)
    {
        // Wrong version
        return RET_WRONG_VERSION;
    }

    if(paket->MsgType >= MSG_MAX_NUM)
    {
        // Wrong message type
        return RET_WRONG_TYPE;
    }

    // Everything OK
    return RET_OK;
}
