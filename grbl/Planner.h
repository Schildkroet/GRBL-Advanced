/*
  Planner.h - buffers movement commands and manages the acceleration profile plan
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
#ifndef PLANNER_H
#define PLANNER_H

#include <stdint.h>
#include "util.h"


// Returned status message from planner.
#define PLAN_OK 						true
#define PLAN_EMPTY_BLOCK 				false

// Define planner data condition flags. Used to denote running conditions of a block.
#define PL_COND_FLAG_RAPID_MOTION      	BIT(0)
#define PL_COND_FLAG_SYSTEM_MOTION     	BIT(1) // Single motion. Circumvents planner state. Used by home/park.
#define PL_COND_FLAG_NO_FEED_OVERRIDE  	BIT(2) // Motion does not honor feed override.
#define PL_COND_FLAG_INVERSE_TIME      	BIT(3) // Interprets feed rate value as inverse time when set.
#define PL_COND_FLAG_SPINDLE_CW        	BIT(4)
#define PL_COND_FLAG_SPINDLE_CCW       	BIT(5)
#define PL_COND_FLAG_COOLANT_FLOOD     	BIT(6)
#define PL_COND_FLAG_COOLANT_MIST      	BIT(7)
#define PL_COND_MOTION_MASK    			(PL_COND_FLAG_RAPID_MOTION|PL_COND_FLAG_SYSTEM_MOTION|PL_COND_FLAG_NO_FEED_OVERRIDE)
#define PL_COND_SPINDLE_MASK            (PL_COND_FLAG_SPINDLE_CW|PL_COND_FLAG_SPINDLE_CCW)
#define PL_COND_ACCESSORY_MASK 			(PL_COND_FLAG_SPINDLE_CW|PL_COND_FLAG_SPINDLE_CCW|PL_COND_FLAG_COOLANT_FLOOD|PL_COND_FLAG_COOLANT_MIST)


// This struct stores a linear movement of a g-code block motion with its critical "nominal" values
// are as specified in the source g-code.
typedef struct {
	// Fields used by the bresenham algorithm for tracing the line
	// NOTE: Used by stepper algorithm to execute the block correctly. Do not alter these values.
	uint32_t steps[N_AXIS];    // Step count along each axis
	uint32_t step_event_count; // The maximum step axis count and number of steps required to complete this block.
	uint8_t direction_bits;    // The direction bit set for this block (refers to *_DIRECTION_BIT in config.h)

	// Block condition data to ensure correct execution depending on states and overrides.
	uint8_t condition;      // Block bitflag variable defining block run conditions. Copied from pl_line_data.
	int32_t line_number;  // Block line number for real-time reporting. Copied from pl_line_data.

	// Fields used by the motion planner to manage acceleration. Some of these values may be updated
	// by the stepper module during execution of special motion cases for replanning purposes.
	float entry_speed_sqr;     // The current planned entry speed at block junction in (mm/min)^2
	float max_entry_speed_sqr; // Maximum allowable entry speed based on the minimum of junction limit and
							//   neighboring nominal speeds with overrides in (mm/min)^2
	float acceleration;        // Axis-limit adjusted line acceleration in (mm/min^2). Does not change.
	float millimeters;         // The remaining distance for this block to be executed in (mm).
							// NOTE: This value may be altered by stepper algorithm during execution.

	// Stored rate limiting data used by planner when changes occur.
	float max_junction_speed_sqr; // Junction entry speed limit based on direction vectors in (mm/min)^2
	float rapid_rate;             // Axis-limit adjusted maximum rate for this block direction in (mm/min)
	float programmed_rate;        // Programmed rate of this block (mm/min).

	// Stored spindle speed data used by spindle overrides and resuming methods.
	float spindle_speed;    // Block spindle speed. Copied from pl_line_data.

	uint8_t backlash_motion;
} Planner_Block_t;


// Planner data prototype. Must be used when passing new motions to the planner.
typedef struct {
	float feed_rate;          // Desired feed rate for line motion. Value is ignored, if rapid motion.
	float spindle_speed;      // Desired spindle speed through line motion.
	uint8_t condition;        // Bitflag variable to indicate planner conditions. See defines above.
	int32_t line_number;    // Desired line number to report when executing.

	uint8_t backlash_motion;
} Planner_LineData_t;


// Initialize and reset the motion plan subsystem
void Planner_Init(void);
void Planner_Reset(void); 		// Reset all
void Planner_ResetBuffer(void); // Reset buffer only.

// Add a new linear movement to the buffer. target[N_AXIS] is the signed, absolute target position
// in millimeters. Feed rate specifies the speed of the motion. If feed rate is inverted, the feed
// rate is taken to mean "frequency" and would complete the operation in 1/feed_rate minutes.
uint8_t Planner_BufferLine(float *target, Planner_LineData_t *pl_data);

// Called when the current block is no longer needed. Discards the block and makes the memory
// availible for new blocks.
void Planner_DiscardCurrentBlock(void);

// Gets the planner block for the special system motion cases. (Parking/Homing)
Planner_Block_t *Planner_GetSystemMotionBlock(void);

// Gets the current block. Returns NULL if buffer empty
Planner_Block_t *Planner_GetCurrentBlock(void);

// Called periodically by step segment buffer. Mostly used internally by planner.
uint8_t Planner_NextBlockIndex(uint8_t block_index);

// Called by step segment buffer when computing executing block velocity profile.
float Planner_GetExecBlockExitSpeedSqr(void);

// Called by main program during planner calculations and step segment buffer during initialization.
float Planner_ComputeProfileNominalSpeed(Planner_Block_t *block);

// Re-calculates buffered motions profile parameters upon a motion-based override change.
void Planner_UpdateVelocityProfileParams(void);

// Reset the planner position vector (in steps)
void Planner_SyncPosition(void);

// Reinitialize plan with a partially completed block
void Planner_CycleReinitialize(void);

// Returns the number of available blocks are in the planner buffer.
uint8_t Planner_GetBlockBufferAvailable(void);

// Returns the number of active blocks are in the planner buffer.
// NOTE: Deprecated. Not used unless classic status reports are enabled in config.h
uint8_t Planner_GetBlockBufferCount(void);

// Returns the status of the block ring buffer. True, if buffer is full.
uint8_t Planner_CheckBufferFull(void);


#endif // PLANNER_H
