/*
  System.h - Header for system level commands and real-time processes
  Part of Grbl-Advanced

  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC
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
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include "util.h"


// Define system executor bit map. Used internally by realtime protocol as realtime command flags,
// which notifies the main program to execute the specified realtime command asynchronously.
// NOTE: The system executor uses an unsigned 8-bit volatile variable (8 flag limit.) The default
// flags are always false, so the realtime protocol only needs to check for a non-zero value to
// know when there is a realtime command to execute.
#define EXEC_STATUS_REPORT  				BIT(0)
#define EXEC_CYCLE_START    				BIT(1)
#define EXEC_CYCLE_STOP     				BIT(2)
#define EXEC_FEED_HOLD      				BIT(3)
#define EXEC_RESET          				BIT(4)
#define EXEC_SAFETY_DOOR    				BIT(5)
#define EXEC_MOTION_CANCEL  				BIT(6)
#define EXEC_SLEEP          				BIT(7)

#define EXEC_FEED_DWELL                     BIT(8)
#define EXEC_TOOL_CHANGE                    BIT(9)

// Alarm executor codes. Valid values (1-255). Zero is reserved.
#define EXEC_ALARM_HARD_LIMIT           	1
#define EXEC_ALARM_SOFT_LIMIT           	2
#define EXEC_ALARM_ABORT_CYCLE          	3
#define EXEC_ALARM_PROBE_FAIL_INITIAL   	4
#define EXEC_ALARM_PROBE_FAIL_CONTACT   	5
#define EXEC_ALARM_HOMING_FAIL_RESET    	6
#define EXEC_ALARM_HOMING_FAIL_DOOR     	7
#define EXEC_ALARM_HOMING_FAIL_PULLOFF  	8
#define EXEC_ALARM_HOMING_FAIL_APPROACH 	9

// Override bit maps. Realtime bitflags to control feed, rapid, spindle, and coolant overrides.
// Spindle/coolant and feed/rapids are separated into two controlling flag variables.
#define EXEC_FEED_OVR_RESET         		BIT(0)
#define EXEC_FEED_OVR_COARSE_PLUS   		BIT(1)
#define EXEC_FEED_OVR_COARSE_MINUS  		BIT(2)
#define EXEC_FEED_OVR_FINE_PLUS     		BIT(3)
#define EXEC_FEED_OVR_FINE_MINUS    		BIT(4)
#define EXEC_RAPID_OVR_RESET        		BIT(5)
#define EXEC_RAPID_OVR_MEDIUM       		BIT(6)
#define EXEC_RAPID_OVR_LOW          		BIT(7)
//#define EXEC_RAPID_OVR_EXTRA_LOW			BIT(*) // *NOT SUPPORTED*

#define EXEC_SPINDLE_OVR_RESET         		BIT(0)
#define EXEC_SPINDLE_OVR_COARSE_PLUS   		BIT(1)
#define EXEC_SPINDLE_OVR_COARSE_MINUS  		BIT(2)
#define EXEC_SPINDLE_OVR_FINE_PLUS     		BIT(3)
#define EXEC_SPINDLE_OVR_FINE_MINUS    		BIT(4)
#define EXEC_SPINDLE_OVR_STOP          		BIT(5)
#define EXEC_COOLANT_FLOOD_OVR_TOGGLE  		BIT(6)
#define EXEC_COOLANT_MIST_OVR_TOGGLE   		BIT(7)

// Define system state bit map. The state variable primarily tracks the individual functions
// of Grbl to manage each without overlapping. It is also used as a messaging flag for
// critical events.
#define STATE_IDLE          				0      // Must be zero. No flags.
#define STATE_ALARM         				BIT(0) // In alarm state. Locks out all g-code processes. Allows settings access.
#define STATE_CHECK_MODE    				BIT(1) // G-code check mode. Locks out planner and motion only.
#define STATE_HOMING        				BIT(2) // Performing homing cycle
#define STATE_CYCLE         				BIT(3) // Cycle is running or motions are being executed.
#define STATE_HOLD          				BIT(4) // Active feed hold
#define STATE_JOG           				BIT(5) // Jogging mode.
#define STATE_SAFETY_DOOR   				BIT(6) // Safety door is ajar. Feed holds and de-energizes system.
#define STATE_SLEEP         				BIT(7) // Sleep state.

#define STATE_FEED_DWELL                    BIT(8)  // Dwell
#define STATE_TOOL_CHANGE                   BIT(9)  // Tool change in progress

// Define system suspend flags. Used in various ways to manage suspend states and procedures.
#define SUSPEND_DISABLE           			0      // Must be zero.
#define SUSPEND_HOLD_COMPLETE     			BIT(0) // Indicates initial feed hold is complete.
#define SUSPEND_RESTART_RETRACT   			BIT(1) // Flag to indicate a retract from a restore parking motion.
#define SUSPEND_RETRACT_COMPLETE  			BIT(2) // (Safety door only) Indicates retraction and de-energizing is complete.
#define SUSPEND_INITIATE_RESTORE  			BIT(3) // (Safety door only) Flag to initiate resume procedures from a cycle start.
#define SUSPEND_RESTORE_COMPLETE  			BIT(4) // (Safety door only) Indicates ready to resume normal operation.
#define SUSPEND_SAFETY_DOOR_AJAR  			BIT(5) // Tracks safety door state for resuming.
#define SUSPEND_MOTION_CANCEL     			BIT(6) // Indicates a canceled resume motion. Currently used by probing routine.
#define SUSPEND_JOG_CANCEL        			BIT(7) // Indicates a jog cancel in process and to reset buffers when complete.

// Define step segment generator state flags.
#define STEP_CONTROL_NORMAL_OP            	0  // Must be zero.
#define STEP_CONTROL_END_MOTION           	BIT(0)
#define STEP_CONTROL_EXECUTE_HOLD         	BIT(1)
#define STEP_CONTROL_EXECUTE_SYS_MOTION   	BIT(2)
#define STEP_CONTROL_UPDATE_SPINDLE_PWM   	BIT(3)

// Define control pin index for Grbl internal use. Pin maps may change, but these values don't.
#define N_CONTROL_PIN 4
#define CONTROL_PIN_INDEX_SAFETY_DOOR   	BIT(0)
#define CONTROL_PIN_INDEX_RESET         	BIT(1)
#define CONTROL_PIN_INDEX_FEED_HOLD     	BIT(2)
#define CONTROL_PIN_INDEX_CYCLE_START   	BIT(3)

// Define spindle stop override control states.
#define SPINDLE_STOP_OVR_DISABLED       	0  // Must be zero.
#define SPINDLE_STOP_OVR_ENABLED        	BIT(0)
#define SPINDLE_STOP_OVR_INITIATE       	BIT(1)
#define SPINDLE_STOP_OVR_RESTORE        	BIT(2)
#define SPINDLE_STOP_OVR_RESTORE_CYCLE  	BIT(3)



// Define global system variables
typedef struct {
	uint16_t state;               // Tracks the current system state of Grbl.
	uint8_t abort;               // System abort flag. Forces exit back to main loop for reset.
	uint8_t suspend;             // System suspend bitflag variable that manages holds, cancels, and safety door.
	uint8_t soft_limit;          // Tracks soft limit errors for the state machine. (boolean)
	uint8_t step_control;        // Governs the step segment generator depending on system state.
	uint8_t probe_succeeded;     // Tracks if last probing cycle was successful.
	uint8_t homing_axis_lock;    // Locks axes when limits engage. Used as an axis motion mask in the stepper ISR.
	uint8_t f_override;          // Feed rate override value in percent
	uint8_t r_override;          // Rapids override value in percent
	uint8_t spindle_speed_ovr;   // Spindle speed value in percent
	uint8_t spindle_stop_ovr;    // Tracks spindle stop override states
	uint8_t report_ovr_counter;  // Tracks when to add override data to status reports.
	uint8_t report_wco_counter;  // Tracks when to add work coordinate offset data to status reports.
#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
	uint8_t override_ctrl;     // Tracks override control states.
#endif
	float spindle_speed;
	uint8_t is_homed;
} System_t;

extern System_t sys;

// NOTE: These position variables may need to be declared as volatiles, if problems arise.
extern int32_t sys_position[N_AXIS];      // Real-time machine (aka home) position vector in steps.
extern int32_t sys_probe_position[N_AXIS]; // Last probe position in machine coordinates and steps.

extern volatile uint8_t sys_probe_state;   // Probing state value.  Used to coordinate the probing cycle with stepper ISR.
extern volatile uint16_t sys_rt_exec_state;   // Global realtime executor bitflag variable for state management. See EXEC bitmasks.
extern volatile uint8_t sys_rt_exec_alarm;   // Global realtime executor bitflag variable for setting various alarms.
extern volatile uint8_t sys_rt_exec_motion_override; // Global realtime executor bitflag variable for motion-based overrides.
extern volatile uint8_t sys_rt_exec_accessory_override; // Global realtime executor bitflag variable for spindle/coolant overrides.


// Initialize the serial protocol
void System_Init(void);

void System_Clear(void);

void System_ResetPosition(void);

// Returns bitfield of control pin states, organized by CONTROL_PIN_INDEX. (1=triggered, 0=not triggered).
uint8_t System_GetControlState(void);

// Returns if safety door is open or closed, based on pin state.
uint8_t System_CheckSafetyDoorAjar(void);

// Executes an internal system command, defined as a string starting with a '$'
uint8_t System_ExecuteLine(char *line);

// Execute the startup script lines stored in EEPROM upon initialization
void System_ExecuteStartup(char *line);

void System_FlagWcoChange(void);

// Returns machine position of axis 'idx'. Must be sent a 'step' array.
float System_ConvertAxisSteps2Mpos(const int32_t *steps, const uint8_t idx);

// Updates a machine 'position' array based on the 'step' array sent.
void System_ConvertArraySteps2Mpos(float *position, const int32_t *steps);

// CoreXY calculation only. Returns x or y-axis "steps" based on CoreXY motor steps.
#ifdef COREXY
int32_t system_convert_corexy_to_x_axis_steps(int32_t *steps);
int32_t system_convert_corexy_to_y_axis_steps(int32_t *steps);
#endif

// Checks and reports if target array exceeds machine travel limits.
uint8_t System_CheckTravelLimits(float *target);

// Special handlers for setting and clearing Grbl's real-time execution flags.
void System_SetExecStateFlag(uint16_t mask);
void System_ClearExecStateFlag(uint16_t mask);
void System_SetExecAlarm(uint8_t code);
void System_ClearExecAlarm(void);
void System_SetExecMotionOverrideFlag(uint8_t mask);
void System_SetExecAccessoryOverrideFlag(uint8_t mask);
void System_ClearExecMotionOverride(void);
void System_ClearExecAccessoryOverrides(void);


#endif // SYSTEM_H
