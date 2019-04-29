#ifndef PRINT_H_INCLUDED
#define PRINT_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif


void Print_Init(void);
int Printf(const char *str, ...);
void PrintFloat(float n, uint8_t decimal_places);
int8_t Getc(char *c);
int Putc(const char c);

void Print_Flush(void);

void PrintFloat_CoordValue(float n);
void PrintFloat_RateValue(float n);


#ifdef __cplusplus
}
#endif


#endif /* PRINT_H_INCLUDED */
