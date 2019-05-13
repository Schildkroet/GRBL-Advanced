/*
 modified 12 Aug 2013
 by Soohwan Kim (suhwan@wiznet.co.kr)
*/
#ifndef ETHERNET_H_INLUDED
#define ETHERNET_H_INLUDED


#include <stdint.h>
#include "util2.h"


#ifdef __cplusplus
extern "C" {
#endif


void Ethernet_Init(uint8_t *mac_address, IPAddress_t *local_ip, IPAddress_t *dns_server, IPAddress_t *gateway, IPAddress_t *subnet);

IPAddress_t Ethernet_LocalIP(void);
IPAddress_t Ethernet_SubnetMask(void);
IPAddress_t Ethernet_GatewayIP(void);
IPAddress_t Ethernet_DnsServerIP(void);

uint8_t Ethernet_LinkStatus(void);


#ifdef __cplusplus
}
#endif


#endif // ETHERNET_H_INLUDED
