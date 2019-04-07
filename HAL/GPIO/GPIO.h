/*
  GPIO.h - GPIO Header
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
#ifndef GPIO_H_INCLUDED
#define GPIO_H_INCLUDED

#include <stdbool.h>
#include "stm32f4xx_gpio.h"

// GPIO Pins
// Stepper Pins
#define GPIO_STEP_X_PORT		GPIOA
#define GPIO_STEP_Y_PORT		GPIOB
#define GPIO_STEP_Z_PORT		GPIOB
#define GPIO_STEP_X_PIN			GPIO_Pin_10
#define GPIO_STEP_Y_PIN			GPIO_Pin_3
#define GPIO_STEP_Z_PIN			GPIO_Pin_5

// Direction Pins
#define GPIO_DIR_X_PORT			GPIOB
#define GPIO_DIR_Y_PORT			GPIOB
#define GPIO_DIR_Z_PORT			GPIOA
#define GPIO_DIR_X_PIN			GPIO_Pin_4
#define GPIO_DIR_Y_PIN			GPIO_Pin_10
#define GPIO_DIR_Z_PIN			GPIO_Pin_8

// Stepper Enable
#define GPIO_ENABLE_PORT		GPIOA
#define GPIO_ENABLE_PIN			GPIO_Pin_9

// Limit Pins
#define GPIO_LIM_X_PORT			GPIOC
#define GPIO_LIM_Y_PORT			GPIOB
#define GPIO_LIM_Z_PORT			GPIOA
#define GPIO_LIM_X_PIN			GPIO_Pin_7
#define GPIO_LIM_Y_PIN			GPIO_Pin_6
#define GPIO_LIM_Z_PIN			GPIO_Pin_6

// Spindle
#define GPIO_SPINDLE_PWM_PORT	GPIOA
#define GPIO_SPINDLE_PWM_PIN	GPIO_Pin_7
#define GPIO_SPINDLE_DIR_PORT	GPIOA
#define GPIO_SPINDLE_DIR_PIN	GPIO_Pin_5
#define GPIO_SPINDLE_ENA_PORT	GPIOB
#define GPIO_SPINDLE_ENA_PIN	GPIO_Pin_7

// Safety door
#define GPIO_DOOR_PORT			GPIOB
#define GPIO_DOOR_PIN			GPIO_Pin_8

// Control pins
#define GPIO_CTRL_RST_PORT		GPIOA
#define GPIO_CTRL_RST_PIN		GPIO_Pin_0
#define GPIO_CTRL_FEED_PORT		GPIOA
#define GPIO_CTRL_FEED_PIN		GPIO_Pin_1
#define GPIO_CTRL_START_PORT	GPIOA
#define GPIO_CTRL_START_PIN		GPIO_Pin_4

// Coolant
#define GPIO_COOL_FLOOD_PORT	GPIOB
#define GPIO_COOL_FLOOD_PIN		GPIO_Pin_0
#define GPIO_COOL_MIST_PORT		GPIOC
#define GPIO_COOL_MIST_PIN		GPIO_Pin_1

// Probe
#define GPIO_PROBE_PORT			GPIOC
#define GPIO_PROBE_PIN			GPIO_Pin_0


#define GPIO_STEPPER	0
#define GPIO_PROBE		1
#define GPIO_SPINDLE	2
#define GPIO_LIMIT		3
#define GPIO_COOLANT	4
#define GPIO_SYSTEM		5


#ifdef __cplusplus
extern "C" {
#endif


void GPIO_InitGPIO(char gpio);


#ifdef __cplusplus
}
#endif


#endif /* GPIO_H_INCLUDED */
