/*
  Probe.h - code pertaining to probing methods
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
#ifndef PROBE_H
#define PROBE_H

#include <stdint.h>


// Values that define the probing state machine.
#define PROBE_OFF           0 // Probing disabled or not in use. (Must be zero.)
#define PROBE_ACTIVE        1 // Actively watching the input pin.

// Probe pin initialization routine.
void Probe_Init(void);

void Probe_Reset(void);

// Called by probe_init() and the mc_probe() routines. Sets up the probe pin invert mask to
// appropriately set the pin logic according to setting for normal-high/normal-low operation
// and the probing cycle modes for toward-workpiece/away-from-workpiece.
void Probe_ConfigureInvertMask(uint8_t is_probe_away);

// Returns probe pin state. Triggered = true. Called by gcode parser and probe state monitor.
uint8_t Probe_GetState(void);

// Monitors probe pin state and records the system position when detected. Called by the
// stepper ISR per ISR tick.
void Probe_StateMonitor(void);


#endif // PROBE_H
