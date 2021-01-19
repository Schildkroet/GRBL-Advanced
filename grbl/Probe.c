/*
  Probe.c - code pertaining to probing methods
  Part of Grbl-Advanced

  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2017 Patrick F.

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
#include <string.h>
#include "Probe.h"
#include "Settings.h"
#include "System.h"
#include "GPIO.h"


// Inverts the probe pin state depending on user settings and probing cycle mode.
static uint8_t probe_invert_mask;


// Probe pin initialization routine.
void Probe_Init(void)
{
    GPIO_InitGPIO(GPIO_PROBE);

    Probe_ConfigureInvertMask(false); // Initialize invert mask.*/
}


void Probe_Reset(void)
{
    // Clear probe position.
    memset(sys_probe_position, 0 , sizeof(sys_probe_position));
}


// Called by probe_init() and the mc_probe() routines. Sets up the probe pin invert mask to
// appropriately set the pin logic according to setting for normal-high/normal-low operation
// and the probing cycle modes for toward-workpiece/away-from-workpiece.
void Probe_ConfigureInvertMask(uint8_t is_probe_away)
{
    probe_invert_mask = 0; // Initialize as zero.

    if(BIT_IS_FALSE(settings.flags, BITFLAG_INVERT_PROBE_PIN))
    {
        probe_invert_mask ^= 1;
    }
    if(is_probe_away)
    {
        probe_invert_mask ^= 1;
    }
}


// Returns the probe pin state. Triggered = true. Called by gcode parser and probe state monitor.
uint8_t Probe_GetState(void)
{
    return (GPIO_ReadInputDataBit(GPIO_PROBE_PORT, GPIO_PROBE_PIN) ^ probe_invert_mask);
}


// Monitors probe pin state and records the system position when detected. Called by the
// stepper ISR per ISR tick.
// NOTE: This function must be extremely efficient as to not bog down the stepper ISR.
void Probe_StateMonitor(void)
{
    if(Probe_GetState())
    {
        sys_probe_state = PROBE_OFF;
        memcpy(sys_probe_position, sys_position, sizeof(sys_position));
        BIT_TRUE(sys_rt_exec_state, EXEC_MOTION_CANCEL);
    }
}
