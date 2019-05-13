/*
 * TcpServer.h
 *
 * Created: 24.01.2018 21:49:39
 *  Author: PatrickVM
 */
#ifndef TCPSERVER_H_
#define TCPSERVER_H_


#include <stdint.h>


uint8_t ServerTCP_Init(uint8_t sock, uint16_t port);
void ServerTCP_DeInit(uint8_t sock);

uint8_t ServerTCP_Send(uint8_t sock, uint8_t *data, uint16_t len);
int32_t ServerTCP_Receive(uint8_t sock, uint8_t *data, uint16_t len);

uint16_t ServerTCP_DataAvailable(uint8_t sock);

void ServerTCP_Update(void);


#endif /* TCPSERVER_H_ */
