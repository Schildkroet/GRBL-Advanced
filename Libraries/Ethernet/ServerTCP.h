/*
 * TcpServer.h
 *
 * Created: 24.01.2018 21:49:39
 *  Author: PatrickVM
 */
#ifndef TCPSERVER_H_
#define TCPSERVER_H_


#include <stdint.h>
#include <stdbool.h>


#define htons(x)    ((uint16_t)(((x) << 8) | (((x) >> 8) & 0xFF)))
#define ntohs(x)    htons(x)

#define htonl(x)    \
                    (((x) << 24 & 0xFF000000UL) | ((x) << 8 & 0x00FF0000UL) | ((x) >> 8 & 0x0000FF00UL) | ((x) >> 24 & 0x000000FFUL))
#define ntohl(x)    htonl(x)


bool ServerTCP_Init(uint8_t sock, uint16_t port);
void ServerTCP_DeInit(uint8_t sock);

int32_t ServerTCP_Send(uint8_t sock, uint8_t *data, uint16_t len);
int32_t ServerTCP_Receive(uint8_t sock, uint8_t *data, uint16_t len);

uint16_t ServerTCP_DataAvailable(uint8_t sock);

void ServerTCP_Update(void);


#endif /* TCPSERVER_H_ */
