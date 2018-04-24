/*
  main.c - An embedded CNC Controller with rs274/ngc (g-code) support
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c)	2017 Patrick F.

  Grbl-Advanced is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Grbl-Advanced is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Grbl-Advanced.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "System32.h"
#include "grbl_advance.h"

#include "Print.h"


// Declare system global variable structure
System_t sys;
int32_t sys_position[N_AXIS];      // Real-time machine (aka home) position vector in steps.
int32_t sys_probe_position[N_AXIS]; // Last probe position in machine coordinates and steps.
volatile uint8_t sys_probe_state;   // Probing state value.  Used to coordinate the probing cycle with stepper ISR.
volatile uint8_t sys_rt_exec_state;   // Global realtime executor bitflag variable for state management. See EXEC bitmasks.
volatile uint8_t sys_rt_exec_alarm;   // Global realtime executor bitflag variable for setting various alarms.
volatile uint8_t sys_rt_exec_motion_override; // Global realtime executor bitflag variable for motion-based overrides.
volatile uint8_t sys_rt_exec_accessory_override; // Global realtime executor bitflag variable for spindle/coolant overrides.


int main(void) {
//	GPIO_InitTypeDef GPIO_InitStructure;
//
//	/* GPIOC clock enable */
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC , ENABLE);
//
//	/* GPIOC Configuration: Test Pin */
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// Init SysTick 1ms
	SysTick_Init();

	// Init formatted output
	Print_Init();

    System_Init();
    Stepper_Init();
    Settings_Init();

    System_ResetPosition();

    if(BIT_IS_TRUE(settings.flags, BITFLAG_HOMING_ENABLE)) {
		sys.state = STATE_ALARM;
    }
    else {
		sys.state = STATE_IDLE;
    }

	// Grbl-Advanced initialization loop upon power-up or a system abort. For the latter, all processes
	// will return to this loop to be cleanly re-initialized.
	while(1) {
		// Reset system variables.
		uint8_t prior_state = sys.state;

		System_Clear();
		sys.state = prior_state;

		Probe_Reset();

		sys_probe_state = 0;
		sys_rt_exec_state = 0;
		sys_rt_exec_alarm = 0;
		sys_rt_exec_motion_override = 0;
		sys_rt_exec_accessory_override = 0;

		// Reset Grbl-Advanced primary systems.
		GC_Init();
		Planner_Init();

		Coolant_Init();
		Limits_Init();
		Probe_Init();
		Spindle_Init();
		Stepper_Reset();

		// Sync cleared gcode and planner positions to current system position.
		Planner_SyncPosition();
		GC_SyncPosition();

		// Print welcome message. Indicates an initialization has occured at power-up or with a reset.
		Report_InitMessage();

		// Start Grbl-Advanced main loop. Processes program inputs and executes them.
		Protocol_MainLoop();
	}

    return 0;
}
