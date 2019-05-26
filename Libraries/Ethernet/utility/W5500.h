/*
  W5500.h - Driver for WIZnet W5500.

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
#ifndef	W5500_H_INCLUDED
#define	W5500_H_INCLUDED


#include "util2.h"


#define MAX_SOCK_NUM        4   // Max 8


#define W5500_SSIZE         2048 // Max Tx buffer size
#define W5500_RSIZE         2048 // Max Rx buffer size


// W5500 Registers
#define REG8_MR             0x0000  // Mode
#define REGN_GAR_4          0x0001  // Gateway IP address
#define REGN_SUBR_4         0x0005  // Subnet mask address
#define REGN_SHAR_6         0x0009  // Source MAC address
#define REGN_SIPR_4         0x000F  // Source IP address
#define REG8_IR             0x0015  // Interrupt
#define REG8_IMR            0x0016  // Interrupt Mask
#define REG16_RTR           0x0019  // Timeout address
#define REG8_RCR            0x001B  // Retry count
#define REGN_UIPR_4         0x0028  // Unreachable IP address in UDP mode
#define REG16_UPORT         0x002C  // Unreachable Port address in UDP mode
#define REG8_PHYCFGR        0x002E  // PHY Configuration register, default value: 0b1011 1XXX


// W5500 Socket registers
#define REG8_SnMR           0x0000  // Mode
#define REG8_SnCR           0x0001  // Command
#define REG8_SnIR           0x0002  // Interrupt
#define REG8_SnSR           0x0003  // Status
#define REG16_SnPORT        0x0004  // Source Port
#define REGN_SnDHAR_6       0x0006  // Destination Hardw Addr
#define REGN_SnDIPR_4       0x000C  // Destination IP Addr
#define REG16_SnDPORT       0x0010  // Destination Port
#define REG16_SnMSSR        0x0012  // Max Segment Size
#define REG8_SnPROTO        0x0014  // Protocol in IP RAW Mode
#define REG8_SnTOS          0x0015  // IP TOS
#define REG8_SnTTL          0x0016  // IP TTL
#define REG16_SnTX_FSR      0x0020  // TX Free Size
#define REG16_SnTX_RD       0x0022  // TX Read Pointer
#define REG16_SnTX_WR       0x0024  // TX Write Pointer
#define REG16_SnRX_RSR      0x0026  // RX Free Size
#define REG16_SnRX_RD       0x0028  // RX Read Pointer
#define REG16_SnRX_WR       0x002A  // RX Write Pointer (supported?)


#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    SnMR_CLOSE  = 0x00,
    SnMR_TCP    = 0x01,
    SnMR_UDP    = 0x02,
    SnMR_IPRAW  = 0x03,
    SnMR_MACRAW = 0x04,
    SnMR_PPPOE  = 0x05,
    SnMR_ND     = 0x20,
    SnMR_MULTI  = 0x80
} SnMR_e;


typedef enum
{
  Sock_OPEN      = 0x01,
  Sock_LISTEN    = 0x02,
  Sock_CONNECT   = 0x04,
  Sock_DISCON    = 0x08,
  Sock_CLOSE     = 0x10,
  Sock_SEND      = 0x20,
  Sock_SEND_MAC  = 0x21,
  Sock_SEND_KEEP = 0x22,
  Sock_RECV      = 0x40
} SockCMD_e;


typedef enum
{
    SnIR_SEND_OK = 0x10,
    SnIR_TIMEOUT = 0x08,
    SnIR_RECV    = 0x04,
    SnIR_DISCON  = 0x02,
    SnIR_CON     = 0x01
} SnIR_e;


typedef enum SnSR
{
    SnSR_CLOSED      = 0x00,
    SnSR_INIT        = 0x13,
    SnSR_LISTEN      = 0x14,
    SnSR_SYNSENT     = 0x15,
    SnSR_SYNRECV     = 0x16,
    SnSR_ESTABLISHED = 0x17,
    SnSR_FIN_WAIT    = 0x18,
    SnSR_CLOSING     = 0x1A,
    SnSR_TIME_WAIT   = 0x1B,
    SnSR_CLOSE_WAIT  = 0x1C,
    SnSR_LAST_ACK    = 0x1D,
    SnSR_UDP         = 0x22,
    SnSR_IPRAW       = 0x32,
    SnSR_MACRAW      = 0x42,
    SnSR_PPPOE       = 0x5F
} SnSR_e;


typedef enum
{
    IPPROTO_IP   = 0,
    IPPROTO_ICMP = 1,
    IPPROTO_IGMP = 2,
    IPPROTO_GGP  = 3,
    IPPROTO_TCP  = 6,
    IPPROTO_PUP  = 12,
    IPPROTO_UDP  = 17,
    IPPROTO_IDP  = 22,
    IPPROTO_ND   = 77,
    IPPROTO_RAW  = 255,
} IPPROTO_e;



void W5500_Init(void);

void W5500_SoftReset(void);

/**
 * @brief	This function is being used for copy the data form Receive buffer of the chip to application buffer.
 *
 * It calculate the actual physical address where one has to read
 * the data from Receive buffer. Here also take care of the condition while it exceed
 * the Rx memory uper-bound of socket.
 */
void W5500_ReadData(SOCKET s, volatile uint16_t src, volatile uint8_t * dst, uint16_t len);

/**
 * @brief	 This function is being called by send() and sendto() function also.
 *
 * This function read the Tx write pointer register and after copy the data in buffer update the Tx write pointer
 * register. User should read upper byte first and lower byte later to get proper value.
 */
void W5500_SendDataProcessing(SOCKET s, const uint8_t *data, uint16_t len);

/**
 * @brief A copy of send_data_processing that uses the provided ptr for the
 *        write offset.  Only needed for the "streaming" UDP API, where
 *        a single UDP packet is built up over a number of calls to
 *        send_data_processing_ptr, because TX_WR doesn't seem to get updated
 *        correctly in those scenarios
 * @param ptr value to use in place of TX_WR.  If 0, then the value is read
 *        in from TX_WR
 * @return New value for ptr, to be used in the next call
 */
void W5500_SendDataProcessingOffset(SOCKET s, uint16_t data_offset, const uint8_t *data, uint16_t len);

/**
 * @brief	This function is being called by recv() also.
 *
 * This function read the Rx read pointer register
 * and after copy the data from receive buffer update the Rx write pointer register.
 * User should read upper byte first and lower byte later to get proper value.
 */
void W5500_RecvDataProcessing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek);

void W5500_SetGatewayIp(uint8_t *_addr);
void W5500_GetGatewayIp(uint8_t *_addr);

void W5500_SetSubnetMask(uint8_t *_addr);
void W5500_GetSubnetMask(uint8_t *_addr);

void W5500_SetMACAddress(uint8_t *_addr);
void W5500_GetMACAddress(uint8_t *_addr);

void W5500_SetIPAddress(uint8_t *_addr);
void W5500_GetIPAddress(uint8_t *_addr);

void W5500_SetRetransmissionTime(uint16_t timeout);
void W5500_SetRetransmissionCount(uint8_t _retry);

void W5500_ExecCmdSn(SOCKET s, SockCMD_e _cmd);

uint16_t W5500_GetTXFreeSize(SOCKET s);
uint16_t W5500_GetRXReceivedSize(SOCKET s);

void W5500_SetPHYCFGR(uint8_t _val);
uint8_t W5500_GetPHYCFGR(void);


// W5500 Registers
// ---------------
void W5500_WRITE_GP_REG8(uint16_t address, uint8_t _data);
uint8_t W5500_READ_GP_REG8(uint16_t address);
void W5500_WRITE_GP_REG16(uint16_t address, uint16_t _data);
uint16_t W5500_READ_GP_REG16(uint16_t address);
uint16_t W5500_WRITE_GP_REGN(uint16_t address, uint8_t *_buff, uint16_t size);
uint16_t W5500_READ_GP_REGN(uint16_t address, uint8_t *_buff, uint16_t size);


// W5500 Socket registers
// ----------------------
void W5500_WRITE_SOCK_REG8(SOCKET _s, uint16_t address, uint8_t _data);
uint8_t W5500_READ_SOCK_REG8(SOCKET _s, uint16_t address);
void W5500_WRITE_SOCK_REG16(SOCKET _s, uint16_t address, uint16_t _data);
uint16_t W5500_READ_SOCK_REG16(SOCKET _s, uint16_t address);
uint16_t W5500_WRITE_SOCK_REGN(SOCKET _s, uint16_t address, uint8_t *_buff, uint16_t size);
uint16_t W5500_READ_SOCK_REGN(SOCKET _s, uint16_t address, uint8_t *_buff, uint16_t size);


#ifdef __cplusplus
}
#endif


#endif
