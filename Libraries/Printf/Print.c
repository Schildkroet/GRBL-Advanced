#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "Print.h"
#include "Config.h"
#include "Usart.h"
#include "FIFO_USART.h"
#include "GrIP.h"
#include "Platform.h"


#define MAX_BUFFER_SIZE     128


static char buf[512] = {0};
static uint16_t buf_idx = 0;


void Printf_Init(void)
{
    Usart_Init(STDOUT, SERIAL_BAUDRATE);
}


int Printf(const char *str, ...)
{
    char buffer[MAX_BUFFER_SIZE];
    uint8_t idx = 0;

    va_list vl;
    va_start(vl, str);
    int i = vsnprintf(buffer, MAX_BUFFER_SIZE, str, vl);

    if(i > MAX_BUFFER_SIZE)
    {
        i = MAX_BUFFER_SIZE;
    }


    for(uint8_t j = 0; j < i; j++)
    {
        buf[buf_idx++] = buffer[j];
    }
    //Usart_Write(STDOUT, false, buffer, i);

    va_end(vl);

    // Return number of sent bytes
    return idx;
}


int8_t Getc(char *c)
{
    if(FifoUsart_Get(STDOUT_NUM, USART_DIR_RX, c) == 0)
    {
        return 0;
    }

    return -1;
}


int Putc(const char c)
{
    buf[buf_idx++] = c;
    //Usart_Put(STDOUT, false, c);

    return 0;
}


void Printf_Flush(void)
{
    if(buf_idx == 0)
    {
        // No data to send
        return;
    }

#if (USE_ETH_IF)
    Pdu_t data;

    data.Data = (uint8_t*)buf;
    data.Length = buf_idx;

    uint8_t ret = GrIP_Transmit(MSG_DATA_NO_RESPONSE, 0, &data);
    (void)ret;  // TODO: Handle transmit error
#else
    Usart_Write(STDOUT, false, buf, buf_idx);
#endif

    memset(buf, 0, 512);
    buf_idx = 0;
}


// Convert float to string by immediately converting to a long integer, which contains
// more digits than a float. Number of decimal places, which are tracked by a counter,
// may be set by the user. The integer is then efficiently converted to a string.
void Printf_Float(float n, uint8_t decimal_places)
{
    if(n < 0)
    {
        Printf("-");
        n = -n;
    }

    uint8_t decimals = decimal_places;

    while(decimals >= 2)   // Quickly convert values expected to be E0 to E-4.
    {
        n *= 100;
        decimals -= 2;
    }

    if(decimals)
    {
        n *= 10;
    }
    n += 0.5; // Add rounding factor. Ensures carryover through entire value.

    // Generate digits backwards and store in string.
    unsigned char buf[13];
    uint8_t i = 0;
    uint32_t a = (long)n;

    while(a > 0)
    {
        buf[i++] = (a % 10) + '0'; // Get digit
        a /= 10;
    }

    while(i < decimal_places)
    {
        buf[i++] = '0'; // Fill in zeros to decimal point for (n < 1)
    }

    if(i == decimal_places)   // Fill in leading zero, if needed.
    {
        buf[i++] = '0';
    }

    // Print the generated string.
    for(; i > 0; i--)
    {
        if(i == decimal_places)
        {
            Printf(".");
        } // Insert decimal point in right place.
        Printf("%c", buf[i - 1]);
    }
}
