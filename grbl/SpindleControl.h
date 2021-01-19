/*
  SpindleControl.h - spindle control methods
  Part of Grbl-Advanced

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
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
#ifndef SPINDLECONTROL_H
#define SPINDLECONTROL_H

#include <stdint.h>


#define SPINDLE_NO_SYNC             false
#define SPINDLE_FORCE_SYNC          true

#define SPINDLE_STATE_DISABLE       0  // Must be zero.
#define SPINDLE_STATE_CW            BIT(0)
#define SPINDLE_STATE_CCW           BIT(1)

#define SPINDLE_PWM_MAX_VALUE       200 // Don't change.
#ifndef SPINDLE_PWM_MIN_VALUE
#define SPINDLE_PWM_MIN_VALUE       1   // Must be greater than zero.
#endif
#define SPINDLE_PWM_OFF_VALUE       1
#define SPINDLE_PWM_RANGE           (SPINDLE_PWM_MAX_VALUE-SPINDLE_PWM_MIN_VALUE)


// Initializes spindle pins and hardware PWM, if enabled.
void Spindle_Init(void);

// Stop and start spindle routines. Called by all spindle routines and stepper ISR.
void Spindle_Stop(void);

// Returns current spindle output state. Overrides may alter it from programmed states.
uint8_t Spindle_GetState(void);

// Called by g-code parser when setting spindle state and requires a buffer sync.
// Immediately sets spindle running state with direction and spindle rpm via PWM, if enabled.
// Called by spindle_sync() after sync and parking motion/spindle stop override during restore.
// Called by g-code parser when setting spindle state and requires a buffer sync.
void Spindle_Sync(uint8_t state, float rpm);

// Sets spindle running state with direction, enable, and spindle PWM.
void Spindle_SetState(uint8_t state, float rpm);

// Sets spindle PWM quickly for stepper ISR. Also called by spindle_set_state().
// NOTE: 328p PWM register is 8-bit.
void Spindle_SetSpeed(uint8_t pwm_value);

// Computes 328p-specific PWM register value for the given RPM for quick updating.
uint8_t Spindle_ComputePwmValue(float rpm);


#endif // SPINDLECONTROL_H
