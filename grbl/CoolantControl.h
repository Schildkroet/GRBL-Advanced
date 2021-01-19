/*
  CoolantControl.h - spindle control methods
  Part of Grbl-Advanced

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2017 Patrick F.

  Grbl is free software: you can redistribute it and/or modify
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
#ifndef COOLANTCONTROL_H
#define COOLANTCONTROL_H


#include <stdint.h>


#define COOLANT_NO_SYNC             false
#define COOLANT_FORCE_SYNC          true

#define COOLANT_STATE_DISABLE       0  // Must be zero
#define COOLANT_STATE_FLOOD         BIT(0)
#define COOLANT_STATE_MIST          BIT(1)


// Initializes coolant control pins.
void Coolant_Init(void);

// Immediately disables coolant pins.
void Coolant_Stop(void);

// Returns current coolant output state. Overrides may alter it from programmed state.
uint8_t Coolant_GetState(void);

// Sets the coolant pins according to state specified.
void Coolant_SetState(uint8_t mode);

// G-code parser entry-point for setting coolant states. Checks for and executes additional conditions.
void Coolant_Sync(uint8_t mode);


#endif // COOLANTCONTROL_H
