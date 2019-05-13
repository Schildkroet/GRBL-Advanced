/*
  GrIP.h - GRBL over IP

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
#ifndef GRIP_H_INCLUDED
#define GRIP_H_INCLUDED


#include <stdint.h>


// Current protocol version
#define GRIP_VERSION            1

// Transmit/Receive buffer size - Do not exceed (GRIP_BUFFER_SIZE - 10)
#define GRIP_BUFFER_SIZE        256
#define GRIP_RX_NUM             3



#ifdef __cplusplus
extern "C" {
#endif


/**
  * Available message types.
  *
  */
typedef enum
{
    MSG_SYSTEM_CMD          = 0,
    MSG_REALTIME_CMD        = 1,
    MSG_DATA                = 2,
    MSG_DATA_NO_RESPONSE    = 3,
    MSG_NOTIFICATION        = 4,
    MSG_RESPONSE            = 5,
    MSG_ERROR               = 6,
    MSG_MAX_NUM             = 7
} MessageType_e;


/**
  * Return Types.
  *
  */
typedef enum
{
    RET_OK                  = 0,
    RET_NOK                 = 1,
    RET_WRONG_VERSION       = 2,
    RET_WRONG_CRC           = 3,
    RET_WRONG_MAGIC         = 4,
    RET_WRONG_PARAM         = 5,
    RET_WRONG_TYPE          = 6
} ReturnType_e;


/**
  * Types for response status.
  *
  */
typedef enum
{
    RESPONSE_OK             = 0,
    RESPONSE_WAIT           = 1,
    RESPONSE_FAIL           = 2
} ResponseStatus_e;


/**
  * GrIP Packet Header
  */
#pragma pack(push, 1)
typedef struct
{
    uint8_t Version;
    uint8_t MsgType;
    uint8_t ReturnCode;
    uint16_t Length;
    uint8_t CRC8;
    uint8_t Counter;
} GrIP_PacketHeader_t;
#pragma pack(pop)


/**
  * GrIP Receive Packet
  */
typedef struct
{
    GrIP_PacketHeader_t RX_Header;
    uint8_t isValid;
    uint8_t Data[GRIP_BUFFER_SIZE];
} RX_Packet_t;


/**
  * Data struct.
  * Data: Pointer to data.
  * Length: Length of data in bytes.
  */
typedef struct
{
    uint8_t *Data;
    uint16_t Length;
} Pdu_t;


/**
  * Initialize the module
  */
void GrIP_Init(void);

/**
  * Transmit a message over GrIP
  */
uint8_t GrIP_Transmit(uint8_t MsgType, uint8_t ReturnCode, Pdu_t *data);

/**
  * Returns the current response state
  */
uint8_t GrIP_ResponseStatus(void);

/**
  * Get data if available
  */
uint8_t GrIP_Receive(RX_Packet_t *pData);

/**
  * Continuously call this function to process RX messages
  */
void GrIP_Update(void);


#ifdef __cplusplus
}
#endif


#endif // GRIP_H_INCLUDED
