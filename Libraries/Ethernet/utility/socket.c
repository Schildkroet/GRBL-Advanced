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
#include "W5500.h"
#include "socket.h"


/**
 * @brief	This Socket function initialize the channel in perticular mode, and set the port and wait for W5100 done it.
 * @return 	1 for success else 0.
 */
int8_t socket(SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag)
{
    if(s < MAX_SOCK_NUM)
    {
        if((protocol == SnMR_TCP) || (protocol == SnMR_UDP) || (protocol == SnMR_IPRAW) || (protocol == SnMR_MACRAW) || (protocol == SnMR_PPPOE))
        {
            close(s);

            W5500_WRITE_SOCK_REG8(s, REG8_SnMR, protocol | flag);

            if(port)
            {
                W5500_WRITE_SOCK_REG16(s, REG16_SnPORT, port);

                W5500_ExecCmdSn(s, Sock_OPEN);

                return SOCK_OK;
            }
            else
            {
                return SOCKERR_PORTZERO;
            }
        }
        else
        {
           return SOCKERR_SOCKMODE;
        }
    }

    return SOCKERR_SOCKNUM;
}


/**
 * @brief	This function close the socket and parameter is "s" which represent the socket number
 */
int8_t close(SOCKET s)
{
    if(s < MAX_SOCK_NUM)
    {
        W5500_ExecCmdSn(s, Sock_CLOSE);
        // Clear interrupts
        W5500_WRITE_SOCK_REG8(s, REG8_SnIR, 0xFF);
    }

    return SOCKERR_SOCKNUM;
}


/**
 * @brief	This function established  the connection for the channel in passive (server) mode. This function waits for the request from the peer.
 * @return	1 for success else 0.
 */
int8_t listen(SOCKET s)
{
    if(s < MAX_SOCK_NUM)
    {
        if(W5500_READ_SOCK_REG8(s, REG8_SnSR) != (uint8_t)SnSR_INIT)
        {
            return SOCKERR_SOCKINIT;
        }

        W5500_ExecCmdSn(s, Sock_LISTEN);

        return SOCK_OK;
    }

    return SOCKERR_SOCKNUM;
}


/**
 * @brief	This function established  the connection for the channel in Active (client) mode.
 * 		This function waits for the untill the connection is established.
 *
 * @return	1 for success else 0.
 */
int8_t connect(SOCKET s, uint8_t * addr, uint16_t port)
{
    if(s < MAX_SOCK_NUM)
    {
        if(((addr[0] == 0xFF) && (addr[1] == 0xFF) && (addr[2] == 0xFF) && (addr[3] == 0xFF)) ||
        ((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) ||
        (port == 0x00))
        {
            return SOCKERR_IPINVALID;
        }

        // Set destination IP
        W5500_WRITE_SOCK_REGN(s, REGN_SnDIPR_4, addr, 4);
        W5500_WRITE_SOCK_REG16(s, REG16_SnDPORT, port);
        W5500_ExecCmdSn(s, Sock_CONNECT);

        return SOCK_OK;
    }

    return SOCKERR_SOCKNUM;
}


/**
 * @brief	This function used for disconnect the socket and parameter is "s" which represent the socket number
 * @return	1 for success else 0.
 */
int8_t disconnect(SOCKET s)
{
    if(s < MAX_SOCK_NUM)
    {
        W5500_ExecCmdSn(s, Sock_DISCON);

        return SOCK_OK;
    }

    return SOCKERR_SOCKNUM;
}


/**
 * @brief	This function used to send the data in TCP mode
 * @return	1 for success else 0.
 */
int32_t send(SOCKET s, const uint8_t * buf, uint16_t len)
{
    uint8_t status = 0;
    uint16_t ret = 0;
    uint16_t freesize = 0;


    if(len > W5500_SSIZE)
        ret = W5500_SSIZE; // check size not to exceed MAX size.
    else
        ret = len;

    if(s < MAX_SOCK_NUM)
    {
        // if freebuf is available, start.
        do
        {
            freesize = W5500_GetTXFreeSize(s);

            status = W5500_READ_SOCK_REG8(s, REG8_SnSR);
            if((status != SnSR_ESTABLISHED) && (status != SnSR_CLOSE_WAIT))
            {
                ret = 0;
                break;
            }
        } while(freesize < ret);

        // copy data
        W5500_SendDataProcessing(s, (uint8_t *)buf, ret);
        W5500_ExecCmdSn(s, Sock_SEND);

        /* +2008.01 bj */
        while((W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_SEND_OK) != SnIR_SEND_OK)
        {
            /* m2008.01 [bj] : reduce code */
            if(W5500_READ_SOCK_REG8(s, REG8_SnSR) == SnSR_CLOSED)
            {
                close(s);

                return SOCKERR_SOCKCLOSED;
            }
        }

        /* +2008.01 bj */
        W5500_WRITE_SOCK_REG8(s, REG8_SnIR, SnIR_SEND_OK);
    }

    return ret;
}


/**
 * @brief	This function is an application I/F function which is used to receive the data in TCP mode.
 * 		It continues to wait for data as much as the application wants to receive.
 *
 * @return	received data size for success else -1.
 */
int32_t recv(SOCKET s, uint8_t *buf, int16_t len)
{
    uint16_t ret = 0;


    if(s < MAX_SOCK_NUM)
    {
        // Check how much data is available
        ret = W5500_GetRXReceivedSize(s);
        if(ret == 0)
        {
            // No data available.
            uint8_t status = W5500_READ_SOCK_REG8(s, REG8_SnSR);
            if(status == SnSR_LISTEN || status == SnSR_CLOSED || status == SnSR_CLOSE_WAIT)
            {
                // The remote end has closed its side of the connection, so this is the eof state
                ret = 0;
            }
            else
            {
                // The connection is still up, but there's no data waiting to be read
                ret = -1;
            }
        }
        else if(ret > len)
        {
            ret = len;
        }

        if(ret > 0)
        {
            W5500_RecvDataProcessing(s, buf, ret, 0);
            W5500_ExecCmdSn(s, Sock_RECV);
        }
    }
    else
    {
        return SOCKERR_SOCKNUM;
    }

    return ret;
}


/**
 * @brief	Returns the first byte in the receive queue (no checking)
 *
 * @return
 */
uint16_t peek(SOCKET s, uint8_t *buf)
{
    if(s < MAX_SOCK_NUM)
    {
        W5500_RecvDataProcessing(s, buf, 1, 1);

        return SOCK_OK;
    }

    return SOCKERR_SOCKNUM;
}


/**
 * @brief	This function is an application I/F function which is used to send the data for other then TCP mode.
 * 		Unlike TCP transmission, The peer's destination address and the port is needed.
 *
 * @return	This function return send data size for success else -1.
 */
int32_t sendto(SOCKET s, const uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t port)
{
    uint16_t ret = 0;


    if(len > W5500_SSIZE)
        ret = W5500_SSIZE; // check size not to exceed MAX size.
    else
        ret = len;

    if(s < MAX_SOCK_NUM)
    {
        if(((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) || ((port == 0x00)) || (ret == 0))
        {
            /* +2008.01 [bj] : added return value */
            ret = 0;
        }
        else
        {
            W5500_WRITE_SOCK_REGN(s, REGN_SnDIPR_4, addr, 4);
            W5500_WRITE_SOCK_REG16(s, REG16_SnDPORT, port);

            // copy data
            W5500_SendDataProcessing(s, (uint8_t *)buf, ret);
            W5500_ExecCmdSn(s, Sock_SEND);

            /* +2008.01 bj */
            while((W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_SEND_OK) != SnIR_SEND_OK)
            {
                if(W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_TIMEOUT)
                {
                    /* +2008.01 [bj]: clear interrupt */
                    W5500_WRITE_SOCK_REG8(s, REG8_SnIR, (SnIR_SEND_OK | SnIR_TIMEOUT));   /* clear SEND_OK & TIMEOUT */

                    return 0;
                }
            }

            /* +2008.01 bj */
            W5500_WRITE_SOCK_REG8(s, REG8_SnIR, SnIR_SEND_OK);
        }
    }

    return ret;
}


/**
 * @brief	This function is an application I/F function which is used to receive the data in other then
 * 	TCP mode. This function is used to receive UDP, IP_RAW and MAC_RAW mode, and handle the header as well.
 *
 * @return	This function return received data size for success else -1.
 */
int32_t recvfrom(SOCKET s, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t *port)
{
    uint8_t head[8];
    uint16_t data_len = 0;
    uint16_t ptr = 0;

    if((s < MAX_SOCK_NUM) && (len > 0))
    {
        ptr = W5500_READ_SOCK_REG16(s, REG16_SnRX_RD);

        switch(W5500_READ_SOCK_REG8(s, REG8_SnMR) & 0x07)
        {
        case SnMR_UDP :
            W5500_ReadData(s, ptr, head, 0x08);
            ptr += 8;
            // read peer's IP address, port number.
            addr[0] = head[0];
            addr[1] = head[1];
            addr[2] = head[2];
            addr[3] = head[3];
            *port = head[4];
            *port = (*port << 8) + head[5];
            data_len = head[6];
            data_len = (data_len << 8) + head[7];

            W5500_ReadData(s, ptr, buf, data_len); // data copy.
            ptr += data_len;

            W5500_WRITE_SOCK_REG16(s, REG16_SnRX_RD, ptr);
            break;

        case SnMR_IPRAW :
            W5500_ReadData(s, ptr, head, 0x06);
            ptr += 6;

            addr[0] = head[0];
            addr[1] = head[1];
            addr[2] = head[2];
            addr[3] = head[3];
            data_len = head[4];
            data_len = (data_len << 8) + head[5];

            W5500_ReadData(s, ptr, buf, data_len); // data copy.
            ptr += data_len;

            W5500_WRITE_SOCK_REG16(s, REG16_SnRX_RD, ptr);
            break;

        case SnMR_MACRAW:
            W5500_ReadData(s, ptr, head, 2);
            ptr+=2;
            data_len = head[0];
            data_len = (data_len<<8) + head[1] - 2;

            W5500_ReadData(s, ptr, buf, data_len);
            ptr += data_len;
            W5500_WRITE_SOCK_REG16(s, REG16_SnRX_RD, ptr);
            break;

        default :
            break;
        }

        W5500_ExecCmdSn(s, Sock_RECV);
    }

    return data_len;
}


/**
 * @brief	Wait for buffered transmission to complete.
 */
void flush(SOCKET s)
{
    if(s < MAX_SOCK_NUM)
    {
        // TODO
        (void)s;
    }
}


uint16_t igmpsend(SOCKET s, const uint8_t * buf, uint16_t len)
{
    //uint8_t status = 0;
    uint16_t ret = 0;


    if(len > W5500_SSIZE)
        ret = W5500_SSIZE; // check size not to exceed MAX size.
    else
        ret = len;

    if(ret == 0)
        return 0;


    if(s < MAX_SOCK_NUM)
    {
        W5500_SendDataProcessing(s, (uint8_t *)buf, ret);
        W5500_ExecCmdSn(s, Sock_SEND);

        while((W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_SEND_OK) != SnIR_SEND_OK)
        {
            //status = W5100_READ_SOCK_REG8(s, REG8_SnSR);
            W5500_READ_SOCK_REG8(s, REG8_SnSR);

            if(W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_TIMEOUT)
            {
                /* in case of igmp, if send fails, then socket closed */
                /* if you want change, remove this code. */
                close(s);

                return 0;
            }
        }

        W5500_WRITE_SOCK_REG8(s, REG8_SnIR, SnIR_SEND_OK);
    }

    return ret;
}


uint16_t bufferData(SOCKET s, uint16_t offset, const uint8_t* buf, uint16_t len)
{
    uint16_t ret = 0;


    if(len > W5500_GetTXFreeSize(s))
    {
        ret = W5500_GetTXFreeSize(s); // check size not to exceed MAX size.
    }
    else
    {
        ret = len;
    }

    W5500_SendDataProcessingOffset(s, offset, buf, ret);

    return ret;
}


int startUDP(SOCKET s, uint8_t* addr, uint16_t port)
{
    if(((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) || ((port == 0x00)))
    {
        return 0;
    }
    else
    {
        W5500_WRITE_SOCK_REGN(s, REGN_SnDIPR_4, addr, 4);
        W5500_WRITE_SOCK_REG16(s, REG16_SnDPORT, port);

        return 1;
    }
}


int sendUDP(SOCKET s)
{
    W5500_ExecCmdSn(s, Sock_SEND);

    /* +2008.01 bj */
    while((W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_SEND_OK) != SnIR_SEND_OK )
    {
        if(W5500_READ_SOCK_REG8(s, REG8_SnIR) & SnIR_TIMEOUT)
        {
            /* +2008.01 [bj]: clear interrupt */
            W5500_WRITE_SOCK_REG8(s, REG8_SnIR, (SnIR_SEND_OK | SnIR_TIMEOUT));

            return 0;
        }
    }

    /* +2008.01 bj */
    W5500_WRITE_SOCK_REG8(s, REG8_SnIR, SnIR_SEND_OK);

    /* Sent ok */
    return 1;
}
