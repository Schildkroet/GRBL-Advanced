/*
  TIM.c - Timer Implementation
  Part of STM32F4_HAL

  Copyright (c)	2017 Patrick F.

  STM32F4_HAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  STM32F4_HAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with STM32F4_HAL.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stm32f4xx_tim.h"
#include "stm32f4xx_gpio.h"
#include "TIM.h"


/**
 * Timer 1
 * Outputs 10 KHz on D11
 * Used for Variable Spindle PWM
 **/
void TIM1_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	/* TIM1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = TIM1_INIT-1;		// ~10 KHz
#ifdef STM32F411xE
	TIM_TimeBaseStructure.TIM_Prescaler = 48-1;		// 2 MHz
#elif STM32F446xx
    TIM_TimeBaseStructure.TIM_Prescaler = 84-1;		// 2 MHz
#else
    #warning No Timer clock set for stepper spindle pwm
#endif
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Set;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);

	/* TIM1 Main Output Enable */
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
}


/**
 * Timer 2
 * Used for Encoder
 **/
void TIM2_Init(void)
{

}


/**
 * Timer 3
 * Used for Input capture
 **/
void TIM3_Init(void)
{
    TIM_ICInitTypeDef  TIM_ICInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    /* TIM3 clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* GPIOC clock enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /* TIM1 channel 4 pin (PC.9) configuration */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Connect TIM pins to AF2 */
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);


    /* Enable the TIM1 global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV2;
    TIM_ICInitStructure.TIM_ICFilter = 0x02;

    TIM_ICInit(TIM3, &TIM_ICInitStructure);

    /* Enable the CC2 Interrupt Request */
    TIM_ITConfig(TIM3, TIM_IT_CC4, ENABLE);

    /* TIM enable counter */
    TIM_Cmd(TIM3, ENABLE);
}


/**
 * Timer 9
 * Base clock: 24 MHz
 * Used for Stepper Interrupt
 * On CC1, Main Stepper Interuppt is called
 * On Update, Stepper Port Reset is called
 **/
void TIM9_Init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	/* TIM9 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
#ifdef STM32F411xE
	TIM_TimeBaseStructure.TIM_Prescaler = 4-1;		// 24 MHz
#elif STM32F446xx
    TIM_TimeBaseStructure.TIM_Prescaler = 7-1;		// 24 MHz
#else
    #warning No Timer clock set for stepper interrupt
#endif
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM9, &TIM_TimeBaseStructure);

	/* Output Compare Toggle Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Active;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_Pulse = 0x0FFF;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC1Init(TIM9, &TIM_OCInitStructure);

    // Enable register preload, to ensure all new register values are only updated after an update event
	TIM_OC1PreloadConfig(TIM9, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM9, ENABLE);

	/* Enable the TIM9 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_TIM9_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* TIM IT enable */
	TIM_ITConfig(TIM9, TIM_IT_CC1 | TIM_IT_Update, ENABLE);

	/* TIM disable counter */
	TIM_Cmd(TIM9, DISABLE);
}
