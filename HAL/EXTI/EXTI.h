#ifndef EXTI_H_INCLUDED
#define EXTI_H_INCLUDED

#include <stdint.h>


#ifdef __cplusplus
 extern "C" {
#endif


void Exti_Init0(uint8_t preemp_prio, uint8_t sub_prio);

void Exti_Init1(uint8_t preemp_prio, uint8_t sub_prio);

void Exti_Init2(uint8_t preemp_prio, uint8_t sub_prio);

void Exti_Init3(uint8_t preemp_prio, uint8_t sub_prio);

void Exti_Init4(uint8_t preemp_prio, uint8_t sub_prio);

// External interrupt for line 5 to 9
void Exti_Init9_5(uint8_t preemp_prio, uint8_t sub_prio);

void Exti_Init15_10(uint8_t preemp_prio, uint8_t sub_prio);


#ifdef __cplusplus
}
#endif


#endif /* EXTI_H_INCLUDED */
