/*
  MotionControl.c - high level interface for issuing motion commands
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
#include <string.h>
#include "Settings.h"
#include "System.h"
#include "Config.h"
#include "GCode.h"
#include "Probe.h"
#include "Limits.h"
#include "Protocol.h"
#include "SpindleControl.h"
#include "Stepper.h"
#include "Report.h"
#include "CoolantControl.h"
#include "MotionControl.h"
#include "defaults.h"


#define DIR_POSITIV     0
#define DIR_NEGATIV     1


static float target_prev[N_AXIS] = {0.0};
static uint8_t dir_negative[N_AXIS] = {DIR_NEGATIV};
static uint8_t backlash_enable = 0;


void MC_Init(void)
{
    for(uint8_t i = 0; i < N_AXIS; i++)
    {
        dir_negative[i] = DIR_NEGATIV ^ (settings.homing_dir_mask & (1<<i));
    }

    MC_SyncBacklashPosition();

	for(uint8_t i = 0; i < N_AXIS; i++)
	{
		// Don't do backlash move, if all axis are 0.0
		if(settings.backlash[i] > 0.0001)
		{
			backlash_enable = 1;
		}
	}
}


void MC_SyncBacklashPosition(void)
{
	// Update target_prev
	System_ConvertArraySteps2Mpos(target_prev, sys_position);
}


// Execute linear motion in absolute millimeter coordinates. Feed rate given in millimeters/second
// unless invert_feed_rate is true. Then the feed_rate means that the motion should be completed in
// (1 minute)/feed_rate time.
// NOTE: This is the primary gateway to the grbl planner. All line motions, including arc line
// segments, must pass through this routine before being passed to the planner. The seperation of
// mc_line and plan_buffer_line is done primarily to place non-planner-type functions from being
// in the planner and to let backlash compensation or canned cycle integration simple and direct.
void MC_Line(float *target, Planner_LineData_t *pl_data)
{
    Planner_LineData_t pl_backlash = {0};
    uint8_t backlash_update = 0;


    pl_backlash.spindle_speed = pl_data->spindle_speed;
    pl_backlash.line_number = pl_data->line_number;
    pl_backlash.feed_rate = pl_data->feed_rate;


	// If enabled, check for soft limit violations. Placed here all line motions are picked up
	// from everywhere in Grbl.
	if(BIT_IS_TRUE(settings.flags, BITFLAG_SOFT_LIMIT_ENABLE)) {
		// NOTE: Block jog state. Jogging is a special case and soft limits are handled independently.
		if(sys.state != STATE_JOG) {
			Limits_SoftCheck(target);
		}
	}

	// If in check gcode mode, prevent motion by blocking planner. Soft limits still work.
	if(sys.state == STATE_CHECK_MODE) {
		return;
	}

	// NOTE: Backlash compensation may be installed here. It will need direction info to track when
	// to insert a backlash line motion(s) before the intended line motion and will require its own
	// plan_check_full_buffer() and check for system abort loop. Also for position reporting
	// backlash steps will need to be also tracked, which will need to be kept at a system level.
	// There are likely some other things that will need to be tracked as well. However, we feel
	// that backlash compensation should NOT be handled by Grbl itself, because there are a myriad
	// of ways to implement it and can be effective or ineffective for different CNC machines. This
	// would be better handled by the interface as a post-processor task, where the original g-code
	// is translated and inserts backlash motions that best suits the machine.
	// NOTE: Perhaps as a middle-ground, all that needs to be sent is a flag or special command that
	// indicates to Grbl what is a backlash compensation motion, so that Grbl executes the move but
	// doesn't update the machine position values. Since the position values used by the g-code
	// parser and planner are separate from the system machine positions, this is doable.


	// If the buffer is full: good! That means we are well ahead of the robot.
	// Remain in this loop until there is room in the buffer.
	do {
		Protocol_ExecuteRealtime(); // Check for any run-time commands

		if(sys.abort) {
			// Bail, if system abort.
			return;
		}

		if(Planner_CheckBufferFull()) {
			// Auto-cycle start when buffer is full.
			Protocol_AutoCycleStart();
		}
		else {
			break;
		}
	} while(1);

#ifdef ENABLE_BACKLASH_COMPENSATION
    pl_backlash.backlash_motion = 1;
    pl_backlash.condition = PL_COND_FLAG_RAPID_MOTION; // Set rapid motion condition flag.

	// Backlash compensation
    for(uint8_t i = 0; i < N_AXIS; i++)
    {
        // Move positive?
        if(target[i] > target_prev[i])
        {
            // Last move negative?
            if(dir_negative[i] == DIR_NEGATIV)
            {
                dir_negative[i] = DIR_POSITIV;
                target_prev[i] += settings.backlash[i];

                backlash_update = 1;
            }
        }
        // Move negative?
        else if(target[i] < target_prev[i])
        {
            // Last move positive?
            if(dir_negative[i] == DIR_POSITIV)
            {
                dir_negative[i] = DIR_NEGATIV;
                target_prev[i] -= settings.backlash[i];

                backlash_update = 1;
            }
        }
    }

    if(backlash_enable && backlash_update)
    {
        // Perform backlash move if necessary
        Planner_BufferLine(target_prev, &pl_backlash);
    }

    memcpy(target_prev, target, N_AXIS*sizeof(float));

    // Backlash move needs a slot in planner buffer, so we have to check again, if planner is free
    do {
		Protocol_ExecuteRealtime(); // Check for any run-time commands

		if(sys.abort) {
			// Bail, if system abort.
			return;
		}

		if(Planner_CheckBufferFull()) {
			// Auto-cycle start when buffer is full.
			Protocol_AutoCycleStart();
		}
		else {
			break;
		}
	} while(1);
#else
	(void)backlash_update;
	(void)pl_backlash;
#endif

	// Plan and queue motion into planner buffer
	if(Planner_BufferLine(target, pl_data) == PLAN_EMPTY_BLOCK) {
		if(BIT_IS_TRUE(settings.flags, BITFLAG_LASER_MODE)) {
			// Correctly set spindle state, if there is a coincident position passed. Forces a buffer
			// sync while in M3 laser mode only.
			if(pl_data->condition & PL_COND_FLAG_SPINDLE_CW) {
				Spindle_Sync(PL_COND_FLAG_SPINDLE_CW, pl_data->spindle_speed);
			}
		}
	}
}

// Execute an arc in offset mode format. position == current xyz, target == target xyz,
// offset == offset from current xyz, axis_X defines circle plane in tool space, axis_linear is
// the direction of helical travel, radius == circle radius, isclockwise boolean. Used
// for vector transformation direction.
// The arc is approximated by generating a huge number of tiny, linear segments. The chordal tolerance
// of each segment is configured in settings.arc_tolerance, which is defined to be the maximum normal
// distance from segment to the circle when the end points both lie on the circle.
void MC_Arc(float *target, Planner_LineData_t *pl_data, float *position, float *offset, float radius,
  uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear, uint8_t is_clockwise_arc)
{
	float center_axis0 = position[axis_0] + offset[axis_0];
	float center_axis1 = position[axis_1] + offset[axis_1];
	float r_axis0 = -offset[axis_0];  // Radius vector from center to current location
	float r_axis1 = -offset[axis_1];
	float rt_axis0 = target[axis_0] - center_axis0;
	float rt_axis1 = target[axis_1] - center_axis1;

	// CCW angle between position and target from circle center. Only one atan2() trig computation required.
	float angular_travel = atan2(r_axis0*rt_axis1-r_axis1*rt_axis0, r_axis0*rt_axis0+r_axis1*rt_axis1);
	if(is_clockwise_arc) { // Correct atan2 output per direction
		if(angular_travel >= -ARC_ANGULAR_TRAVEL_EPSILON) {
			angular_travel -= 2*M_PI;
		}
	}
	else {
		if(angular_travel <= ARC_ANGULAR_TRAVEL_EPSILON) {
			angular_travel += 2*M_PI;
		}
	}

	// NOTE: Segment end points are on the arc, which can lead to the arc diameter being smaller by up to
	// (2x) settings.arc_tolerance. For 99% of users, this is just fine. If a different arc segment fit
	// is desired, i.e. least-squares, midpoint on arc, just change the mm_per_arc_segment calculation.
	// For the intended uses of Grbl, this value shouldn't exceed 2000 for the strictest of cases.
	uint16_t segments = floor(fabs(0.5*angular_travel*radius) / sqrt(settings.arc_tolerance*(2*radius - settings.arc_tolerance)));

	if(segments) {
		// Multiply inverse feed_rate to compensate for the fact that this movement is approximated
		// by a number of discrete segments. The inverse feed_rate should be correct for the sum of
		// all segments.
		if (pl_data->condition & PL_COND_FLAG_INVERSE_TIME) {
			pl_data->feed_rate *= segments;
			BIT_FALSE(pl_data->condition, PL_COND_FLAG_INVERSE_TIME); // Force as feed absolute mode over arc segments.
		}

		float theta_per_segment = angular_travel / segments;
		float linear_per_segment = (target[axis_linear] - position[axis_linear])/segments;

		/* Vector rotation by transformation matrix: r is the original vector, r_T is the rotated vector,
		and phi is the angle of rotation. Solution approach by Jens Geisler.
			r_T = [cos(phi) -sin(phi);
					sin(phi)  cos(phi] * r ;

		For arc generation, the center of the circle is the axis of rotation and the radius vector is
		defined from the circle center to the initial position. Each line segment is formed by successive
		vector rotations. Single precision values can accumulate error greater than tool precision in rare
		cases. So, exact arc path correction is implemented. This approach avoids the problem of too many very
		expensive trig operations [sin(),cos(),tan()] which can take 100-200 usec each to compute.

		Small angle approximation may be used to reduce computation overhead further. A third-order approximation
		(second order sin() has too much error) holds for most, if not, all CNC applications. Note that this
		approximation will begin to accumulate a numerical drift error when theta_per_segment is greater than
		~0.25 rad(14 deg) AND the approximation is successively used without correction several dozen times. This
		scenario is extremely unlikely, since segment lengths and theta_per_segment are automatically generated
		and scaled by the arc tolerance setting. Only a very large arc tolerance setting, unrealistic for CNC
		applications, would cause this numerical drift error. However, it is best to set N_ARC_CORRECTION from a
		low of ~4 to a high of ~20 or so to avoid trig operations while keeping arc generation accurate.

		This approximation also allows mc_arc to immediately insert a line segment into the planner
		without the initial overhead of computing cos() or sin(). By the time the arc needs to be applied
		a correction, the planner should have caught up to the lag caused by the initial mc_arc overhead.
		This is important when there are successive arc motions.
		*/
		// Computes: cos_T = 1 - theta_per_segment^2/2, sin_T = theta_per_segment - theta_per_segment^3/6) in ~52usec
		float cos_T = 2.0 - theta_per_segment*theta_per_segment;
		float sin_T = theta_per_segment*0.16666667*(cos_T + 4.0);
		cos_T *= 0.5;

		float sin_Ti;
		float cos_Ti;
		float r_axisi;
		uint16_t i;
		uint8_t count = 0;

		for(i = 1; i < segments; i++) { // Increment (segments-1).
			if(count < N_ARC_CORRECTION) {
				// Apply vector rotation matrix. ~40 usec
				r_axisi = r_axis0*sin_T + r_axis1*cos_T;
				r_axis0 = r_axis0*cos_T - r_axis1*sin_T;
				r_axis1 = r_axisi;
				count++;
			}
			else {
				// Arc correction to radius vector. Computed only every N_ARC_CORRECTION increments. ~375 usec
				// Compute exact location by applying transformation matrix from initial radius vector(=-offset).
				cos_Ti = cos(i*theta_per_segment);
				sin_Ti = sin(i*theta_per_segment);
				r_axis0 = -offset[axis_0]*cos_Ti + offset[axis_1]*sin_Ti;
				r_axis1 = -offset[axis_0]*sin_Ti - offset[axis_1]*cos_Ti;
				count = 0;
			}

			// Update arc_target location
			position[axis_0] = center_axis0 + r_axis0;
			position[axis_1] = center_axis1 + r_axis1;
			position[axis_linear] += linear_per_segment;

			MC_Line(position, pl_data);

			// Bail mid-circle on system abort. Runtime command check already performed by mc_line.
			if(sys.abort) {
				return;
			}
		}
	}

	// Ensure last segment arrives at target location.
	MC_Line(target, pl_data);
}


// Execute dwell in seconds.
void MC_Dwell(float seconds)
{
	if(sys.state == STATE_CHECK_MODE) {
		return;
	}

    // TODO: DWELL sys.state
	Protocol_BufferSynchronize();
	Delay_sec(seconds, DELAY_MODE_DWELL);
}


// Perform homing cycle to locate and set machine zero. Only '$H' executes this command.
// NOTE: There should be no motions in the buffer and Grbl must be in an idle state before
// executing the homing cycle. This prevents incorrect buffered plans after homing.
void MC_HomigCycle(uint8_t cycle_mask)
{
	// Check and abort homing cycle, if hard limits are already enabled. Helps prevent problems
	// with machines with limits wired on both ends of travel to one limit pin.
	// TODO: Move the pin-specific LIMIT_PIN call to limits.c as a function.
#ifdef LIMITS_TWO_SWITCHES_ON_AXES
	if(Limits_GetState()) {
		MC_Reset(); // Issue system reset and ensure spindle and coolant are shutdown.
		System_SetExecAlarm(EXEC_ALARM_HARD_LIMIT);

		return;
	}
#endif

	Limits_Disable(); // Disable hard limits pin change register for cycle duration

	// -------------------------------------------------------------------------------------
	// Perform homing routine. NOTE: Special motion case. Only system reset works.

#ifdef HOMING_SINGLE_AXIS_COMMANDS
	if(cycle_mask) {
		// Perform homing cycle based on mask.
		Limits_GoHome(cycle_mask);
	}
	else
#endif
	{
		(void)cycle_mask;
		// Search to engage all axes limit switches at faster homing seek rate.
		Limits_GoHome(HOMING_CYCLE_0);  // Homing cycle 0
#ifdef HOMING_CYCLE_1
		Limits_GoHome(HOMING_CYCLE_1);  // Homing cycle 1
#endif

#ifdef HOMING_CYCLE_2
		Limits_GoHome(HOMING_CYCLE_2);  // Homing cycle 2
#endif
	}

	Protocol_ExecuteRealtime(); // Check for reset and set system abort.
	if(sys.abort) {
		// Did not complete. Alarm state set by mc_alarm.
		return;
	}

	// Homing cycle complete! Setup system for normal operation.
	// -------------------------------------------------------------------------------------

	// Sync gcode parser and planner positions to homed position.
	GC_SyncPosition();
	Planner_SyncPosition();

	// If hard limits feature enabled, re-enable hard limits pin change register after homing cycle.
	Limits_Init();
}


// Perform tool length probe cycle. Requires probe switch.
// NOTE: Upon probe failure, the program will be stopped and placed into ALARM state.
uint8_t MC_ProbeCycle(float *target, Planner_LineData_t *pl_data, uint8_t parser_flags)
{
	// TODO: Need to update this cycle so it obeys a non-auto cycle start.
	if(sys.state == STATE_CHECK_MODE) {
		return GC_PROBE_CHECK_MODE;
	}

	// Finish all queued commands and empty planner buffer before starting probe cycle.
	Protocol_BufferSynchronize();
	if(sys.abort) {
		// Return if system reset has been issued.
		return GC_PROBE_ABORT;
	}

	// Initialize probing control variables
	uint8_t is_probe_away = BIT_IS_TRUE(parser_flags,GC_PARSER_PROBE_IS_AWAY);
	uint8_t is_no_error = BIT_IS_TRUE(parser_flags,GC_PARSER_PROBE_IS_NO_ERROR);

	sys.probe_succeeded = false; // Re-initialize probe history before beginning cycle.
	Probe_ConfigureInvertMask(is_probe_away);

	// After syncing, check if probe is already triggered. If so, halt and issue alarm.
	// NOTE: This probe initialization error applies to all probing cycles.
	if(Probe_GetState()) { // Check probe pin state.
		System_SetExecAlarm(EXEC_ALARM_PROBE_FAIL_INITIAL);
		Protocol_ExecuteRealtime();
		Probe_ConfigureInvertMask(false); // Re-initialize invert mask before returning.

		return GC_PROBE_FAIL_INIT; // Nothing else to do but bail.
	}

	// Setup and queue probing motion. Auto cycle-start should not start the cycle.
	MC_Line(target, pl_data);

	// Activate the probing state monitor in the stepper module.
	sys_probe_state = PROBE_ACTIVE;

	// Perform probing cycle. Wait here until probe is triggered or motion completes.
	System_SetExecStateFlag(EXEC_CYCLE_START);
	do {
		Protocol_ExecuteRealtime();

		if(sys.abort) {
			// Check for system abort
			return(GC_PROBE_ABORT);
		}
	} while(sys.state != STATE_IDLE);

	// Probing cycle complete!

	// Set state variables and error out, if the probe failed and cycle with error is enabled.
	if(sys_probe_state == PROBE_ACTIVE) {
		if(is_no_error) {
			memcpy(sys_probe_position, sys_position, sizeof(sys_position));
		}
		else {
			System_SetExecAlarm(EXEC_ALARM_PROBE_FAIL_CONTACT);
		}
	}
	else {
		sys.probe_succeeded = true; // Indicate to system the probing cycle completed successfully.
	}

	sys_probe_state = PROBE_OFF; // Ensure probe state monitor is disabled.
	Probe_ConfigureInvertMask(false); // Re-initialize invert mask.
	Protocol_ExecuteRealtime();   // Check and execute run-time commands

	// Reset the stepper and planner buffers to remove the remainder of the probe motion.
	Stepper_Reset(); // Reset step segment buffer.
	Planner_Reset(); // Reset planner buffer. Zero planner positions. Ensure probing motion is cleared.
	Planner_SyncPosition(); // Sync planner position to current machine position.
	MC_SyncBacklashPosition();

#ifdef MESSAGE_PROBE_COORDINATES
	// All done! Output the probe position as message.
	Report_ProbeParams();
#endif

	if(sys.probe_succeeded) {
		// Successful probe cycle.
		return GC_PROBE_FOUND;
	}
	else {
		return GC_PROBE_FAIL_END;
	} // Failed to trigger probe within travel. With or without error.
}


#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
void MC_OverrideCtrlUpdate(uint8_t override_state)
{
    // Finish all queued commands before altering override control state
    Protocol_BufferSynchronize();

    if(sys.abort) {
		return;
	}

    sys.override_ctrl = override_state;
}
#endif


// Plans and executes the single special motion case for parking. Independent of main planner buffer.
// NOTE: Uses the always free planner ring buffer head to store motion parameters for execution.
#ifdef PARKING_ENABLE
void MC_ParkingMotion(float *parking_target, Planner_LineData_t *pl_data)
{
    if(sys.abort) {
		// Block during abort.
		return;
	}

    uint8_t plan_status = Planner_BufferLine(parking_target, pl_data);

    if(plan_status) {
		BIT_TRUE(sys.step_control, STEP_CONTROL_EXECUTE_SYS_MOTION);
		BIT_FALSE(sys.step_control, STEP_CONTROL_END_MOTION); // Allow parking motion to execute, if feed hold is active.
		Stepper_ParkingSetupBuffer(); // Setup step segment buffer for special parking motion case
		Stepper_PrepareBuffer();
		Stepper_WakeUp();

		do {
			Protocol_ExecRtSystem();

			if(sys.abort) {
				return;
			}
		} while (sys.step_control & STEP_CONTROL_EXECUTE_SYS_MOTION);

		Stepper_ParkingRestoreBuffer(); // Restore step segment buffer to normal run state.
    }
    else {
		BIT_FALSE(sys.step_control, STEP_CONTROL_EXECUTE_SYS_MOTION);
		Protocol_ExecRtSystem();
    }

}
#endif


// Method to ready the system to reset by setting the realtime reset command and killing any
// active processes in the system. This also checks if a system reset is issued while Grbl
// is in a motion state. If so, kills the steppers and sets the system alarm to flag position
// lost, since there was an abrupt uncontrolled deceleration. Called at an interrupt level by
// realtime abort command and hard limits. So, keep to a minimum.
void MC_Reset(void)
{
	// Only this function can set the system reset. Helps prevent multiple kill calls.
	if(BIT_IS_FALSE(sys_rt_exec_state, EXEC_RESET)) {
		System_SetExecStateFlag(EXEC_RESET);

		// Kill spindle and coolant.
		Spindle_Stop();
		Coolant_Stop();

		// Kill steppers only if in any motion state, i.e. cycle, actively holding, or homing.
		// NOTE: If steppers are kept enabled via the step idle delay setting, this also keeps
		// the steppers enabled by avoiding the go_idle call altogether, unless the motion state is
		// violated, by which, all bets are off.
		if((sys.state & (STATE_CYCLE | STATE_HOMING | STATE_JOG)) || (sys.step_control & (STEP_CONTROL_EXECUTE_HOLD | STEP_CONTROL_EXECUTE_SYS_MOTION))) {
			if(sys.state == STATE_HOMING) {
				if(!sys_rt_exec_alarm) {
					System_SetExecAlarm(EXEC_ALARM_HOMING_FAIL_RESET);
				}
			}
			else {
				System_SetExecAlarm(EXEC_ALARM_ABORT_CYCLE);
			}

			Stepper_Disable(0); // Force kill steppers. Position has likely been lost.
		}
	}
}
