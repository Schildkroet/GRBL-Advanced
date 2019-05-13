/*
 modified 12 Aug 2013
 by Soohwan Kim (suhwan@wiznet.co.kr)
*/
#include "Ethernet.h"
#include "W5500.h"


IPAddress_t _dnsServerAddress = {{0, 0, 0, 0}};


void Ethernet_Init(uint8_t *mac, IPAddress_t *local_ip, IPAddress_t *dns_server, IPAddress_t *gateway, IPAddress_t *subnet)
{
    W5500_Init();
    W5500_SetMACAddress(mac);
    W5500_SetIPAddress(local_ip->IP);
    W5500_SetGatewayIp(gateway->IP);
    W5500_SetSubnetMask(subnet->IP);

    _dnsServerAddress = *dns_server;
}


IPAddress_t Ethernet_LocalIP(void)
{
    IPAddress_t ret;

    W5500_GetIPAddress(ret.IP);

    return ret;
}


IPAddress_t Ethernet_SubnetMask(void)
{
    IPAddress_t ret;

    W5500_GetSubnetMask(ret.IP);

    return ret;
}


IPAddress_t Ethernet_GatewayIP(void)
{
    IPAddress_t ret;

    W5500_GetGatewayIp(ret.IP);

    return ret;
}


IPAddress_t Ethernet_DnsServerIP(void)
{
    return _dnsServerAddress;
}


uint8_t Ethernet_LinkStatus(void)
{
    return (W5500_GetPHYCFGR() & 0x01);
}
