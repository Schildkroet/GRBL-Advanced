/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    06-March-2015
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "Usart.h"
#include "FIFO_USART.h"
#include "Limits.h"
#include "Stepper.h"
#include "System.h"
#include "Settings.h"
#include "Config.h"
#include "MotionControl.h"
#include "Encoder.h"
#include "Platform.h"
#include "TIM.h"
#include <stdbool.h>


#define RPM_FILTER_NUM      3


/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile uint8_t DebounceCounterControl = 0;
volatile uint8_t DebounceCounterLimits = 0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

extern void Limit_PinChangeISR(void);
extern void System_PinChangeISR(void);


// Counter for milliseconds
static volatile uint32_t gMillis = 0;

uint32_t spindle_rpm = 0;
uint16_t tim4_cnt_prev = 0;
uint32_t rpm_arr[RPM_FILTER_NUM] = {0};
uint8_t rpm_idx = 0;


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

uint32_t millis(void)
{
	return gMillis;
}


void ProcessReceive(char c)
{
    // Pick off realtime command characters directly from the serial stream. These characters are
	// not passed into the main buffer, but these set system state flag bits for realtime execution.
	switch(c)
	{
	case CMD_RESET:         MC_Reset(); break; // Call motion control reset routine.
	case CMD_RESET_HARD:    NVIC_SystemReset();     // Perform hard reset
	case CMD_STATUS_REPORT: System_SetExecStateFlag(EXEC_STATUS_REPORT);break;
	case CMD_CYCLE_START:   System_SetExecStateFlag(EXEC_CYCLE_START); break; // Set as true
	case CMD_FEED_HOLD:     System_SetExecStateFlag(EXEC_FEED_HOLD); break; // Set as true
	case CMD_STEPPER_DISABLE:     Stepper_Disable(1); break; // Set as true

	default:
		if(c > 0x7F) { // Real-time control characters are extended ACSII only.
			switch(c)
			{
			case CMD_SAFETY_DOOR: System_SetExecStateFlag(EXEC_SAFETY_DOOR); break; // Set as true
			case CMD_JOG_CANCEL:
				if(sys.state & STATE_JOG) { // Block all other states from invoking motion cancel.
					System_SetExecStateFlag(EXEC_MOTION_CANCEL);
				}
				break;

			case CMD_FEED_OVR_RESET: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_RESET); break;
			case CMD_FEED_OVR_COARSE_PLUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_COARSE_PLUS); break;
			case CMD_FEED_OVR_COARSE_MINUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_COARSE_MINUS); break;
			case CMD_FEED_OVR_FINE_PLUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_FINE_PLUS); break;
			case CMD_FEED_OVR_FINE_MINUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_FINE_MINUS); break;
			case CMD_RAPID_OVR_RESET: System_SetExecMotionOverrideFlag(EXEC_RAPID_OVR_RESET); break;
			case CMD_RAPID_OVR_MEDIUM: System_SetExecMotionOverrideFlag(EXEC_RAPID_OVR_MEDIUM); break;
			case CMD_RAPID_OVR_LOW: System_SetExecMotionOverrideFlag(EXEC_RAPID_OVR_LOW); break;
			case CMD_SPINDLE_OVR_RESET: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_RESET); break;
			case CMD_SPINDLE_OVR_COARSE_PLUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_COARSE_PLUS); break;
			case CMD_SPINDLE_OVR_COARSE_MINUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_COARSE_MINUS); break;
			case CMD_SPINDLE_OVR_FINE_PLUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_FINE_PLUS); break;
			case CMD_SPINDLE_OVR_FINE_MINUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_FINE_MINUS); break;
			case CMD_SPINDLE_OVR_STOP: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_STOP); break;
			case CMD_COOLANT_FLOOD_OVR_TOGGLE: System_SetExecAccessoryOverrideFlag(EXEC_COOLANT_FLOOD_OVR_TOGGLE); break;
#ifdef ENABLE_M7
            case CMD_COOLANT_MIST_OVR_TOGGLE: System_SetExecAccessoryOverrideFlag(EXEC_COOLANT_MIST_OVR_TOGGLE); break;
#endif
			}
		// Throw away any unfound extended-ASCII character by not passing it to the serial buffer.
		}
		else {
			// Write character to buffer
			FifoUsart_Insert(USART2_NUM, USART_DIR_RX, c);
		}
	}
}


/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}


/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}


/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}


/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}


/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	/*
	 * Because of the board layout, we cant attach all pins to interrupts.
	 * Therefore we just poll them in this 1ms task, which is hopefully fast
	 * enough for critical events. Debouncing pins is also implemented here.
	 */
	uint8_t limits = Limits_GetState();
	if(limits)
    {
		// X-Y-Z Limit
		if((DebounceCounterLimits == 0) && (settings.system_flags & BITFLAG_ENABLE_LIMITS))
        {
			DebounceCounterLimits = 20;
			Limit_PinChangeISR();
		}
	}

	uint8_t controls = System_GetControlState();
	if(controls)
    {
		// System control
		if(DebounceCounterControl == 0)
        {
			DebounceCounterControl = 20;
			System_PinChangeISR();
		}
	}

	if(DebounceCounterLimits && !limits)
    {
		DebounceCounterLimits--;
	}
	if(DebounceCounterControl && !controls)
    {
		DebounceCounterControl--;
	}

	gMillis++;

  if(gMillis%16 == 0)
  {
    // Update sync motion
    MC_UpdateSyncMove();
  }

	if(gMillis%25 == 0)
    {
        // 25ms Task (min 7 RPM)
        uint16_t cnt = (uint16_t)Encoder_GetValue();
        uint32_t cnt_diff = 0;

        // Calculate ticks since last event
        if(cnt < tim4_cnt_prev)
        {
            // Ovf
            cnt_diff = (0xFFFF - tim4_cnt_prev) + cnt;
        }
        else
        {
            cnt_diff = cnt - tim4_cnt_prev;
        }

        // Calculate RPM and smooth it
        float rpm = ((cnt_diff * 40.0) / PULSES_PER_REV) * 60.0;
        rpm_arr[rpm_idx++] = (uint32_t)rpm;
        if(rpm_idx > (RPM_FILTER_NUM-1))
        {
            rpm_idx = 0;
        }

        // Assign smoothed RPM
        spindle_rpm = (rpm_arr[0] + rpm_arr[1] + rpm_arr[2]) / RPM_FILTER_NUM;

        tim4_cnt_prev = cnt;

        // Update sync motion
        //MC_UpdateSyncMove();
    }
}


/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles TIM9 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM1_BRK_TIM9_IRQHandler(void)
{
	/* TIM9_CH1 */
	if(TIM_GetITStatus(TIM9, TIM_IT_CC1) != RESET)
    {
		// OC
		Stepper_MainISR();

		TIM_ClearITPendingBit(TIM9, TIM_IT_CC1);
	}
	else if(TIM_GetITStatus(TIM9, TIM_IT_Update) != RESET)
    {
		// OVF
		Stepper_PortResetISR();

		TIM_ClearITPendingBit(TIM9, TIM_IT_Update);
	}
}


void TIM3_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM3, TIM_IT_CC4) == SET)
    {
        /* Clear TIM3 Capture compare interrupt pending bit */
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC4);
    }
}


void TIM4_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
		// OVF
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

		Encoder_OvfISR();

        // Spindle at zero position
        if(sys.sync_move && sys.state == STATE_HOLD)
        {
            MC_LineSyncStart();
        }
	}
}


/**
  * @brief  This function handles External lines 9 to 5 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI9_5_IRQHandler(void)
{
}


/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		/* Read one byte from the receive data register */
		unsigned char c = (USART_ReceiveData(USART1) & 0xFF);

		// Write character to buffer
		FifoUsart_Insert(USART1_NUM, USART_DIR_RX, c);
	}

	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		char c;

		if(FifoUsart_Get(USART1_NUM, USART_DIR_TX, &c) == 0) {
			/* Write one byte to the transmit data register */
			while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
			USART_SendData(USART1, c);
		}
		else {
			// Nothing to transmit - disable interrupt
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}

	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) {
		(void)USART_ReceiveData(USART1);
	}
}


/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		/* Read one byte from the receive data register */
		unsigned char c = (USART_ReceiveData(USART2) & 0xFF);

		ProcessReceive(c);
	}

	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		char c;

		if(FifoUsart_Get(USART2_NUM, USART_DIR_TX, &c) == 0) {
			/* Write one byte to the transmit data register */
			while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
			USART_SendData(USART2, c);
		}
		else {
			// Nothing to transmit - disable interrupt
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		}
	}

	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if(USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET) {
		(void)USART_ReceiveData(USART2);
	}
}


/**
  * @brief  This function handles USART6 global interrupt request.
  * @param  None
  * @retval None
  */
void USART6_IRQHandler(void)
{
	if(USART_GetITStatus(USART6, USART_IT_RXNE) != RESET) {
		/* Read one byte from the receive data register */
		unsigned char c = (USART_ReceiveData(USART6) & 0xFF);

		// Write character to buffer
		FifoUsart_Insert(USART6_NUM, USART_DIR_RX, c);
	}

	if(USART_GetITStatus(USART6, USART_IT_TXE) != RESET) {
		char c;

		if(FifoUsart_Get(USART6_NUM, USART_DIR_TX, &c) == 0) {
			/* Write one byte to the transmit data register */
			while(USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET);
			USART_SendData(USART6, c);
		}
		else {
			// Nothing to transmit - disable interrupt
			USART_ITConfig(USART6, USART_IT_TXE, DISABLE);
		}
	}

	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if(USART_GetFlagStatus(USART6, USART_FLAG_ORE) != RESET) {
		(void)USART_ReceiveData(USART6);
	}
}

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
