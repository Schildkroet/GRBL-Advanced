#ifndef UTIL2_H
#define UTIL2_H


#include <stdint.h>
#include <stdlib.h>


#define htons(x)        ((uint16_t)(((x)<<8) | (((x)>>8)&0xFF)))
#define ntohs(x)        htons(x)

#define htonl(x)        (((x)<<24 & 0xFF000000UL) | ((x)<< 8 & 0x00FF0000UL) | ((x)>> 8 & 0x0000FF00UL) | ((x)>>24 & 0x000000FFUL))
#define ntohl(x)        htonl(x)


#ifdef __cplusplus
extern "C" {
#endif


typedef uint8_t SOCKET;


typedef struct
{
    uint8_t IP[4];
} IPAddress_t;

static const IPAddress_t INADDR_NONE = {0};


#ifdef __cplusplus
}
#endif


#endif
