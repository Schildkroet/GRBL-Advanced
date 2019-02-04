/*
  MotionControl.h - high level interface for issuing motion commands
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
#ifndef MOTIONCONTROL_H
#define MOTIONCONTROL_H

#include <stdint.h>
#include "Planner.h"

// System motion commands must have a line number of zero.
#define HOMING_CYCLE_LINE_NUMBER 		0
#define PARKING_MOTION_LINE_NUMBER 		0

#define HOMING_CYCLE_ALL  				0  // Must be zero.
#define HOMING_CYCLE_X   				BIT(X_AXIS)
#define HOMING_CYCLE_Y   				BIT(Y_AXIS)
#define HOMING_CYCLE_Z   				BIT(Z_AXIS)


void MC_Init(void);

void MC_SyncBacklashPosition(void);


// Execute linear motion in absolute millimeter coordinates. Feed rate given in millimeters/second
// unless invert_feed_rate is true. Then the feed_rate means that the motion should be completed in
// (1 minute)/feed_rate time.
void MC_Line(float *target, Planner_LineData_t *pl_data);

// Execute an arc in offset mode format. position == current xyz, target == target xyz,
// offset == offset from current xyz, axis_XXX defines circle plane in tool space, axis_linear is
// the direction of helical travel, radius == circle radius, is_clockwise_arc boolean. Used
// for vector transformation direction.
void MC_Arc(float *target, Planner_LineData_t *pl_data, float *position, float *offset, float radius,
  uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear, uint8_t is_clockwise_arc);

// Dwell for a specific number of seconds
void MC_Dwell(float seconds);

// Perform homing cycle to locate machine zero. Requires limit switches.
void MC_HomigCycle(uint8_t cycle_mask);

// Perform tool length probe cycle. Requires probe switch.
uint8_t MC_ProbeCycle(float *target, Planner_LineData_t *pl_data, uint8_t parser_flags);

// Handles updating the override control state.
void MC_OverrideCtrlUpdate(uint8_t override_state);

// Plans and executes the single special motion case for parking. Independent of main planner buffer.
void MC_ParkingMotion(float *parking_target, Planner_LineData_t *pl_data);

// Performs system reset. If in motion state, kills all motion and sets system alarm.
void MC_Reset(void);


#endif // MOTIONCONTROL_H
