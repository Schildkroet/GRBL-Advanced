/*
 * TcpServer.c
 *
 * Created: 24.01.2018 21:49:52
 *  Author: PatrickVM
 */
#include "ServerTCP.h"
#include "W5500.h"
#include "socket.h"
#include "System32.h"


static uint8_t mSock = 0;
static uint16_t mPort = 0;

uint8_t ServerTCP_Init(uint8_t sock, uint16_t port)
{
    mSock = sock;
	mPort = port;

	// Check if socket is available
	if(W5500_READ_SOCK_REG8(sock, REG8_SnSR) == SnSR_CLOSED)
	{
	    // Set socket to TCP listen mode
		socket(sock, SnMR_TCP, port, 0);
		listen(sock);

		// OK
		return 0;
	}

	// Socket occupied
	return 1;
}


void ServerTCP_DeInit(uint8_t sock)
{
	// Release socket
	disconnect(sock);

	Delay_ms(5);

	// Check if socket is released
	if(W5500_READ_SOCK_REG8(sock, REG8_SnSR) != SnSR_CLOSED)
	{
		// Force release
		close(sock);
	}
}


uint8_t ServerTCP_Send(uint8_t sock, uint8_t *data, uint16_t len)
{
	// Check if socket available
	if(W5500_READ_SOCK_REG8(sock, REG8_SnSR) == SnSR_ESTABLISHED)
	{
		// Send data
		if(send(sock, data, len) <= 0)
		{
			return 1;
		}

		// OK
		return 0;
	}

	return 2;
}


int32_t ServerTCP_Receive(uint8_t sock, uint8_t *data, uint16_t len)
{
	// Check if data is available
	if(W5500_GetRXReceivedSize(sock))
	{
		// Read data
		return recv(sock, data, len);
	}

    // No data available
	return -1;
}


uint16_t ServerTCP_DataAvailable(uint8_t sock)
{
    return W5500_GetRXReceivedSize(sock);
}


void ServerTCP_Update(void)
{
    // If socket is closed, reinitalize it.
    uint8_t ret = 0;

    ret = W5500_READ_SOCK_REG8(mSock, REG8_SnSR);

	if(ret == SnSR_CLOSE_WAIT)
	{
		ServerTCP_DeInit(mSock);
		ServerTCP_Init(mSock, mPort);
	}

	if(ret == SnSR_CLOSED)
    {
        ServerTCP_Init(mSock, mPort);
    }
}
