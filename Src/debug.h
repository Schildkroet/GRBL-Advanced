#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <stdio.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

// Debugging level
#define DEBUG   	1
#define VERBOSE		1


// Get filename
#ifdef _WIN32
    #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif // _WIN32


// VS stuff...
#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif


// Print Debug
#if DEBUG==1
    #define d_printf(...)           do { printf(__VA_ARGS__); } while(0)
#elif DEBUG==2
    #define d_printf(...)           do { printf("%s:%d:%s(): ", __FILENAME__, __LINE__, __func__); printf(__VA_ARGS__); } while(0)
#else
    #define d_printf(...)           do {} while(0)
#endif // DEBUG

// Print Verbose
#if VERBOSE==1
	#define v_printf(x)				d_printf(x)
#else
	#define v_printf(...)			do {} while(0)
#endif // VERBOSE


#ifdef __cplusplus
}
#endif


#endif // DEBUG_H_INCLUDED
