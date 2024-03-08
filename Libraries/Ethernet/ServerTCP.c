/*
 * TcpServer.c
 *
 * Created: 24.01.2018 21:49:52
 *  Author: PatrickVM
 */
#include "ServerTCP.h"
#include "wizchip_conf.h"
#include "SPI.h"
#include "socket.h"
#include "System32.h"
#include "Platform.h"
#include "Print.h"


// Debug output
//#define _LOOPBACK_DEBUG_

#define ETH_MAX_BUF_SIZE        32


static int32_t loopback_tcp_server(uint8_t sn, uint8_t *buf, uint16_t port);
static void PrintNetworkInfo(void);


static uint8_t mSock = 0;
static uint16_t mPort = 0;


static uint8_t wiznet_memsize[2][8] = {{4, 2, 2, 2, 2, 2, 1, 1}, {4, 2, 2, 2, 2, 2, 1, 1}};

static uint8_t ethBuf0[ETH_MAX_BUF_SIZE];
static wiz_NetInfo gWIZNETINFO = {.mac = {0x00, 0x08, 0xdc, 0x11, 0x22, 0x33},
                                  .ip = {192, 168, 1, 20},
                                  .sn = {255, 255, 255, 0},
                                  .gw = {192, 168, 1, 1},
                                  .dns = {8, 8, 8, 8},
                                  .dhcp = NETINFO_STATIC};


bool ServerTCP_Init(uint8_t sock, uint16_t port)
{
    mSock = sock;
	mPort = port;

    Spi_Init(SPI_W5500, SPI_MODE0);

    // Set clock to 21 Mhz (W5500 should support up to about 80 Mhz)
    Spi_SetPrescaler(SPI_W5500, SPI_PRESCALER_2);

    // Hard reset
    GPIO_ResetBits(GPIOA, GPIO_Pin_15);
    Delay_ms(40);
    GPIO_SetBits(GPIOA, GPIO_Pin_15);
    Delay_ms(40);

    //wizchip_sw_reset();
    reg_wizchip_spi_cbfunc(0, 0);
    reg_wizchip_spiburst_cbfunc(0, 0);
    reg_wizchip_cs_cbfunc(0, 0);

    if (ctlwizchip(CW_INIT_WIZCHIP, (void*)wiznet_memsize) == -1)
    {
#ifdef _LOOPBACK_DEBUG_
        Printf("WIZchip memory initialization failed\r\n");
        Printf_Flush();
#endif
        return false;
    }

    ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);

    // Uncomment if phy cant establish a link
    //wiz_PhyConf conf = {PHY_CONFBY_SW, PHY_MODE_MANUAL, PHY_SPEED_10, PHY_DUPLEX_FULL};
    //wizphy_setphyconf(&conf);

    /*uint8_t tmp;
    do
    {
        if (ctlwizchip(CW_GET_PHYLINK, (void *)&tmp) == -1)
        {
#ifdef _LOOPBACK_DEBUG_
            Printf("Unknown PHY Link status.\r\n");
            Printf_Flush();
#endif
            return false;
        }
    } while (tmp == PHY_LINK_OFF);*/

#ifdef _LOOPBACK_DEBUG_
    Printf("version:%.2X\r\n", getVERSIONR());
    Printf_Flush();
#endif

    return true;
}


void ServerTCP_DeInit(uint8_t sock)
{
	// Release socket
	disconnect(sock);

	Delay_ms(5);

	// Check if socket is released
    if (getSn_SR(sock) != SOCK_CLOSED)
    {
		// Force release
		close(sock);
	}
}


int32_t ServerTCP_Send(uint8_t sock, uint8_t *data, uint16_t len)
{
	int32_t ret = send(sock, data, len);

    if (ret < 0)
    {
#if defined(_LOOPBACK_DEBUG_)
        Printf("%d: send() error: %ld\r\n", sock, ret);
        Printf_Flush();
#endif
        close(sock);
    }

    return ret;
}


int32_t ServerTCP_Receive(uint8_t sock, uint8_t *data, uint16_t len)
{
    int32_t ret = 0;
    uint16_t size = ServerTCP_DataAvailable(sock);

    if (size)
    {
        ret = recv(sock, data, len);

        if (ret != size)
        {
            if (ret == SOCK_BUSY)
            {
                return 0;
            }
            if (ret < 0)
            {
#if defined(_LOOPBACK_DEBUG_)
                Printf("%d: recv() error: %ld\r\n", sock, ret);
                Printf_Flush();
#endif
                close(sock);
            }
        }
        return ret;
    }

    // No data available
	return 0;
}


uint16_t ServerTCP_DataAvailable(uint8_t sock)
{
    return getSn_RX_RSR(sock);
}


void ServerTCP_Update(void)
{
    loopback_tcp_server(mSock, ethBuf0, mPort);
}


static int32_t loopback_tcp_server(uint8_t sn, uint8_t *buf, uint16_t port)
{
    int32_t ret;
    uint8_t tmp;

    (void)buf;

#ifdef _LOOPBACK_DEBUG_
    uint8_t destip[4];
    uint16_t destport;
#endif

    if (ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
    {
        // Error
        return -1;
    }
    else
    {
        if (tmp == PHY_LINK_OFF)
        {
            // No active link
            return 0;
        }
    }

    switch (getSn_SR(sn))
    {
    case SOCK_ESTABLISHED:
        if (getSn_IR(sn) & Sn_IR_CON)
        {
#ifdef _LOOPBACK_DEBUG_
            getSn_DIPR(sn, destip);
            destport = getSn_DPORT(sn);

            Printf("%d:Connected - %d.%d.%d.%d : %d\r\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
#endif
            setSn_IR(sn, Sn_IR_CON);
        }

        // Don't need to check SOCKERR_BUSY because it doesn't not occur.
        /*uint16_t size = 0;
        uint16_t sentsize = 0;

        if ((size = getSn_RX_RSR(sn)) > 0)
        {
            if (size > DATA_BUF_SIZE)
            {
                size = DATA_BUF_SIZE;
            }
            ret = recv(sn, buf, size);

            if (ret <= 0)
            {
                // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                return ret;
            }
            size = (uint16_t)ret;
            sentsize = 0;

            while (size != sentsize)
            {
                ret = send(sn, buf + sentsize, size - sentsize);
                if (ret < 0)
                {
                    close(sn);
                    return ret;
                }
                // Don't care SOCKERR_BUSY, because it is zero.
                sentsize += ret;
            }
        }*/
        break;

    case SOCK_CLOSE_WAIT:
#ifdef _LOOPBACK_DEBUG_
        Printf("%d:CloseWait\r\n", sn);
        Printf_Flush();
#endif
        if ((ret = disconnect(sn)) != SOCK_OK)
        {
            return ret;
        }
#ifdef _LOOPBACK_DEBUG_
        Printf("%d:Socket Closed\r\n", sn);
#endif
        break;

    case SOCK_INIT:
#ifdef _LOOPBACK_DEBUG_
        Printf("%d:Listen, TCP server loopback, port [%d]\r\n", sn, port);
        Printf_Flush();
#endif
        if ((ret = listen(sn)) != SOCK_OK)
        {
            return ret;
        }
        break;

    case SOCK_CLOSED:
#ifdef _LOOPBACK_DEBUG_
        Printf("%d:TCP server loopback start\r\n", sn);
#endif
        if ((ret = socket(sn, Sn_MR_TCP, port, 0x00)) != sn)
        {
            return ret;
        }
#ifdef _LOOPBACK_DEBUG_
        Printf("%d:Socket opened\r\n", sn);
#endif
        break;

    default:
        break;
    }

#ifdef _LOOPBACK_DEBUG_
    Printf_Flush();
#endif

    return 1;
}


__attribute__((unused))
static void PrintNetworkInfo(void)
{
    wiz_NetInfo defaultNetInfo;

    wizchip_getnetinfo(&defaultNetInfo);
    Printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\r",
           defaultNetInfo.mac[0],
           defaultNetInfo.mac[1],
           defaultNetInfo.mac[2],
           defaultNetInfo.mac[3],
           defaultNetInfo.mac[4],
           defaultNetInfo.mac[5]);
    Printf("IP address : %d.%d.%d.%d\n\r",
           defaultNetInfo.ip[0],
           defaultNetInfo.ip[1],
           defaultNetInfo.ip[2],
           defaultNetInfo.ip[3]);
    Printf("SM Mask	   : %d.%d.%d.%d\n\r",
           defaultNetInfo.sn[0],
           defaultNetInfo.sn[1],
           defaultNetInfo.sn[2],
           defaultNetInfo.sn[3]);
    Printf("Gate way   : %d.%d.%d.%d\n\r",
           defaultNetInfo.gw[0],
           defaultNetInfo.gw[1],
           defaultNetInfo.gw[2],
           defaultNetInfo.gw[3]);
    Printf("DNS Server : %d.%d.%d.%d\n\r",
           defaultNetInfo.dns[0],
           defaultNetInfo.dns[1],
           defaultNetInfo.dns[2],
           defaultNetInfo.dns[3]);
    Printf_Flush();
}
