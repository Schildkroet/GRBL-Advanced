#include "EXTI.h"
#include "stm32f4xx_conf.h"



/* Private function prototypes -----------------------------------------------*/
static void Exti_RFM22B_RecvPacket(void);



void Exti_Init0(uint8_t preemp_prio, uint8_t sub_prio)
{
	// Suppress compiler warning
	(void)preemp_prio;
	(void)sub_prio;
}

void Exti_Init1(uint8_t preemp_prio, uint8_t sub_prio)
{
	(void)preemp_prio;
	(void)sub_prio;
}

void Exti_Init2(uint8_t preemp_prio, uint8_t sub_prio)
{
	(void)preemp_prio;
	(void)sub_prio;
}

void Exti_Init3(uint8_t preemp_prio, uint8_t sub_prio)
{
	(void)preemp_prio;
	(void)sub_prio;
}

void Exti_Init4(uint8_t preemp_prio, uint8_t sub_prio)
{
	(void)preemp_prio;
	(void)sub_prio;
}

// External interrupt for line 5 to 9
void Exti_Init9_5(uint8_t preemp_prio, uint8_t sub_prio)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable and set EXTI9_5 Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = preemp_prio;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = sub_prio;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	Exti_RFM22B_RecvPacket();
}

void Exti_Init15_10(uint8_t preemp_prio, uint8_t sub_prio)
{
	(void)preemp_prio;
	(void)sub_prio;
}

//-------------------------------------------------------------------------------------------------//

static void Exti_RFM22B_RecvPacket(void)
{
	EXTI_InitTypeDef   EXTI_InitStructure;

	// Connect EXTI8 Line to PC8 pin
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, GPIO_PinSource8);

	// Configure EXTI8 line
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}
