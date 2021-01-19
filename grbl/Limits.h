/*
  limits.h - code pertaining to limit-switches and performing the homing cycle
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
#ifndef LIMITS_H
#define LIMITS_H

#include <stdint.h>


// Initialize the limits module
void Limits_Init(void);

// Disables hard limits.
void Limits_Disable(void);

// Returns limit state as a bit-wise uint8 variable.
uint8_t Limits_GetState(void);

void Limit_PinChangeISR(void);

// Perform one portion of the homing cycle based on the input settings.
void Limits_GoHome(uint8_t cycle_mask);

// Check for soft limit violations
void Limits_SoftCheck(float *target);


#endif // LIMITS_H
