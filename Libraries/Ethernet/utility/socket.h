/*
  socket.c - SOCKET APIs like as berkeley socket api.

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
#ifndef	_SOCKET_H_
#define	_SOCKET_H_


#include <stdint.h>
#include "util.h"


#define SOCK_OK                     1                     ///< Result is OK about socket process.
#define SOCK_BUSY                   0                     ///< Socket is busy on processing the operation. Valid only Non-block IO Mode.
#define SOCK_FATAL                  -1000                 ///< Result is fatal error about socket process.

#define SOCK_ERROR                  0
#define SOCKERR_SOCKNUM             (SOCK_ERROR - 1)      ///< Invalid socket number
#define SOCKERR_SOCKOPT             (SOCK_ERROR - 2)      ///< Invalid socket option
#define SOCKERR_SOCKINIT            (SOCK_ERROR - 3)      ///< Socket is not initialized
#define SOCKERR_SOCKCLOSED          (SOCK_ERROR - 4)      ///< Socket unexpectedly closed.
#define SOCKERR_SOCKMODE            (SOCK_ERROR - 5)      ///< Invalid socket mode for socket operation.
#define SOCKERR_SOCKFLAG            (SOCK_ERROR - 6)      ///< Invalid socket flag
#define SOCKERR_SOCKSTATUS          (SOCK_ERROR - 7)      ///< Invalid socket status for socket operation.
#define SOCKERR_ARG                 (SOCK_ERROR - 10)     ///< Invalid argrument.
#define SOCKERR_PORTZERO            (SOCK_ERROR - 11)     ///< Port number is zero
#define SOCKERR_IPINVALID           (SOCK_ERROR - 12)     ///< Invalid IP address
#define SOCKERR_TIMEOUT             (SOCK_ERROR - 13)     ///< Timeout occurred
#define SOCKERR_DATALEN             (SOCK_ERROR - 14)     ///< Data length is zero or greater than buffer max size.
#define SOCKERR_BUFFER              (SOCK_ERROR - 15)     ///< Socket buffer is not enough for data communication.

#define SOCKFATAL_PACKLEN           (SOCK_FATAL - 1)      ///< Invalid packet length. Fatal Error.


#define SF_IO_NONBLOCK              0x01              ///< Socket nonblock io mode. It used parameter in \ref socket().

/*
 * UDP & MACRAW Packet Infomation
 */
#define PACK_FIRST                  0x80              ///< In Non-TCP packet, It indicates to start receiving a packet.
#define PACK_REMAINED               0x01              ///< In Non-TCP packet, It indicates to remain a packet to be received.
#define PACK_COMPLETED              0x00              ///< In Non-TCP packet, It indicates to complete to receive a packet.


#ifdef __cplusplus
extern "C" {
#endif


// Opens a socket(TCP or UDP or IP_RAW mode)
int8_t socket(SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag);
// Close socket
int8_t close(SOCKET s);

// Establish TCP connection (Active connection)
int8_t connect(SOCKET s, uint8_t * addr, uint16_t port);
// disconnect the connection
int8_t disconnect(SOCKET s);

// Establish TCP connection (Passive connection)
int8_t listen(SOCKET s);

// Send data (TCP)
int32_t send(SOCKET s, const uint8_t * buf, uint16_t len);
// Receive data (TCP)
int32_t recv(SOCKET s, uint8_t * buf, int16_t len);

uint16_t peek(SOCKET s, uint8_t *buf);

// Send data (UDP/IP RAW)
int32_t sendto(SOCKET s, const uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t port);
// Receive data (UDP/IP RAW)
int32_t recvfrom(SOCKET s, uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t *port);

// Wait for transmission to complete
void flush(SOCKET s);



uint16_t igmpsend(SOCKET s, const uint8_t * buf, uint16_t len);

// Functions to allow buffered UDP send (i.e. where the UDP datagram is built up over a
// number of calls before being sent
/*
  @brief This function sets up a UDP datagram, the data for which will be provided by one
  or more calls to bufferData and then finally sent with sendUDP.
  @return 1 if the datagram was successfully set up, or 0 if there was an error
*/
int startUDP(SOCKET s, uint8_t* addr, uint16_t port);
/*
  @brief This function copies up to len bytes of data from buf into a UDP datagram to be
  sent later by sendUDP.  Allows datagrams to be built up from a series of bufferData calls.
  @return Number of bytes successfully buffered
*/
uint16_t bufferData(SOCKET s, uint16_t offset, const uint8_t* buf, uint16_t len);
/*
  @brief Send a UDP datagram built up from a sequence of startUDP followed by one or more
  calls to bufferData.
  @return 1 if the datagram was successfully sent, or 0 if there was an error
*/
int sendUDP(SOCKET s);


#ifdef __cplusplus
}
#endif


#endif /* _SOCKET_H_ */
