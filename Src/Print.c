#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "Print.h"
#include "Config.h"
#include "USART.h"
#include "FIFO_USART.h"
#include "Settings.h"


void Print_Init(void)
{
	Usart_Init(STDOUT, BAUD_RATE);
}

int Printf(const char *str, ...)
{
	char buffer[128];
	uint8_t idx = 0;

	va_list vl;
	va_start(vl, str);
	uint8_t i = vsprintf(buffer, str, vl);

	while(i) {
		while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
		USART_SendData(USART2, buffer[idx++]);

		i--;
	}

	va_end(vl);

	return 0;
}

// Convert float to string by immediately converting to a long integer, which contains
// more digits than a float. Number of decimal places, which are tracked by a counter,
// may be set by the user. The integer is then efficiently converted to a string.
// NOTE: AVR '%' and '/' integer operations are very efficient. Bitshifting speed-up
// techniques are actually just slightly slower. Found this out the hard way.
void PrintFloat(float n, uint8_t decimal_places)
{
	if(n < 0) {
		Putc('-');
		n = -n;
	}

	uint8_t decimals = decimal_places;

	while(decimals >= 2) { // Quickly convert values expected to be E0 to E-4.
		n *= 100;
		decimals -= 2;
	}

	if(decimals) {
		n *= 10;
	}
	n += 0.5; // Add rounding factor. Ensures carryover through entire value.

	// Generate digits backwards and store in string.
	unsigned char buf[13];
	uint8_t i = 0;
	uint32_t a = (long)n;

	while(a > 0) {
		buf[i++] = (a % 10) + '0'; // Get digit
		a /= 10;
	}

	while(i < decimal_places) {
		buf[i++] = '0'; // Fill in zeros to decimal point for (n < 1)
	}

	if(i == decimal_places) { // Fill in leading zero, if needed.
		buf[i++] = '0';
	}

	// Print the generated string.
	for(; i > 0; i--) {
		if(i == decimal_places) {
			Putc('.');
		} // Insert decimal point in right place.
		Putc(buf[i-1]);
	}
}

int8_t Getc(char *c)
{
	if(FifoUsart_Get(STDOUT_NUM, USART_DIR_RX, c) == 0) {
		return 0;
	}

	return -1;
}

int Putc(const char c)
{
	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
	USART_SendData(USART2, c);

	return 0;
}

// Floating value printing handlers for special variables types used in Grbl and are defined
// in the config.h.
//  - CoordValue: Handles all position or coordinate values in inches or mm reporting.
//  - RateValue: Handles feed rate and current velocity in inches or mm reporting.
void PrintFloat_CoordValue(float n)
{
	if(BIT_IS_TRUE(settings.flags, BITFLAG_REPORT_INCHES)) {
		PrintFloat(n*INCH_PER_MM,N_DECIMAL_COORDVALUE_INCH);
	}
	else {
		PrintFloat(n, N_DECIMAL_COORDVALUE_MM);
	}
}

void PrintFloat_RateValue(float n)
{
	if(BIT_IS_TRUE(settings.flags, BITFLAG_REPORT_INCHES)) {
		PrintFloat(n*INCH_PER_MM,N_DECIMAL_RATEVALUE_INCH);
	}
	else {
		PrintFloat(n, N_DECIMAL_RATEVALUE_MM);
	}
}
