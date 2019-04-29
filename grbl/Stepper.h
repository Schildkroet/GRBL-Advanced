/*
  Stepper.h - stepper motor driver: executes motion plans of planner.c using the stepper motors
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
#ifndef STEPPER_H
#define STEPPER_H


// Initialize and setup the stepper motor subsystem
void Stepper_Init(void);

// Enable steppers, but cycle does not start unless called by motion control or realtime command.
void Stepper_WakeUp(void);

// Immediately disables steppers
void Stepper_Disable(uint8_t ovr_disable);

// Main ISR
void Stepper_MainISR(void);

// Stepper Port Reset ISR
void Stepper_PortResetISR(void);

// Generate the step and direction port invert masks.
void Stepper_GenerateStepDirInvertMasks(void);

// Reset the stepper subsystem variables
void Stepper_Reset(void);

// Changes the run state of the step segment buffer to execute the special parking motion.
void Stepper_ParkingSetupBuffer(void);

// Restores the step segment buffer to the normal run state after a parking motion.
void Stepper_ParkingRestoreBuffer(void);

// Reloads step segment buffer. Called continuously by realtime execution system.
void Stepper_PrepareBuffer(void);

// Called by planner_recalculate() when the executing block is updated by the new plan.
void Stepper_UpdatePlannerBlockParams(void);

// Called by realtime status reporting if realtime rate reporting is enabled in config.h.
float Stepper_GetRealtimeRate(void);


#endif // STEPPER_H
