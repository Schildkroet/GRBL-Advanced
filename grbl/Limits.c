/*
  limits.c - code pertaining to limit-switches and performing the homing cycle
  Part of Grbl-Advanced

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c) 2017-2020 Patrick F.

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
#include "System.h"
#include "Planner.h"
#include "MotionControl.h"
#include "Config.h"
#include "Settings.h"
#include "Stepper.h"
#include "Protocol.h"
#include "Limits.h"
#include "GPIO.h"
#include "System32.h"


// Homing axis search distance multiplier. Computed by this value times the cycle travel.
#ifndef HOMING_AXIS_SEARCH_SCALAR
    #define HOMING_AXIS_SEARCH_SCALAR   1.5 // Must be > 1 to ensure limit switch will be engaged.
#endif
#ifndef HOMING_AXIS_LOCATE_SCALAR
    #define HOMING_AXIS_LOCATE_SCALAR   5.0 // Must be > 1 to ensure limit switch is cleared.
#endif


static uint8_t last_state = 0;


void Limits_Init(void)
{
    GPIO_InitGPIO(GPIO_LIMIT);
    last_state = 0;

    if (BIT_IS_TRUE(settings.flags, BITFLAG_HARD_LIMIT_ENABLE))
    {
        // Enable hard limits
        sys.system_flags |= BITFLAG_ENABLE_LIMITS;
    }
    else
    {
        Limits_Disable();
    }
}


void Limits_Disable(void)
{
    // Disables hard limits.
    sys.system_flags &= ~BITFLAG_ENABLE_LIMITS;
}


// Returns limit state as a bit-wise uint8 variable. Each bit indicates an axis limit, where
// triggered is 1 and not triggered is 0. Invert mask is applied. Axes are defined by their
// number in bit position, i.e. Z_AXIS is (1<<2) or bit 2, and Y_AXIS is (1<<1) or bit 1.
uint8_t Limits_GetState(bool held)
{
    uint8_t limit_state = 0;

    // First limit
    limit_state = (GPIO_ReadInputDataBit(GPIO_LIM_X_PORT, GPIO_LIM_X_PIN) << X1_LIMIT_BIT);
    limit_state |= (GPIO_ReadInputDataBit(GPIO_LIM_Y_PORT, GPIO_LIM_Y_PIN) << Y1_LIMIT_BIT);
    limit_state |= (GPIO_ReadInputDataBit(GPIO_LIM_Z_PORT, GPIO_LIM_Z_PIN) << Z1_LIMIT_BIT);

    // Second limit
    limit_state |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)<<X2_LIMIT_BIT);
    limit_state |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5)<<Y2_LIMIT_BIT);
    limit_state |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6)<<Z2_LIMIT_BIT);

    if(BIT_IS_TRUE(settings.flags, BITFLAG_INVERT_LIMIT_PINS))
    {
        limit_state ^= LIMIT_MASK;
    }

    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE))
    {
        // Clear Y-Limits in lathe mode
        limit_state &= ~(1 << Y1_LIMIT_BIT | 1 << Y2_LIMIT_BIT);
    }

    uint8_t tmp_state = limit_state ^ last_state;
    tmp_state &= limit_state;

    if (held)
    {
        tmp_state = limit_state;
    }

    last_state = limit_state;

    return tmp_state;
}


// This is the Limit Pin Change Interrupt, which handles the hard limit feature. A bouncing
// limit switch can cause a lot of problems, like false readings and multiple interrupt calls.
// If a switch is triggered at all, something bad has happened and treat it as such, regardless
// if a limit switch is being disengaged. It's impossible to reliably tell the state of a
// bouncing pin because the Arduino microcontroller does not retain any state information when
// detecting a pin change. If we poll the pins in the ISR, you can miss the correct reading if the
// switch is bouncing.
// NOTE: Do not attach an e-stop to the limit pins, because this interrupt is disabled during
// homing cycles and will not respond correctly. Upon user request or need, there may be a
// special pinout for an e-stop, but it is generally recommended to just directly connect
// your e-stop switch to the Arduino reset pin, since it is the most correct way to do this.
void Limit_PinChangeISR(void) // DEFAULT: Limit pin change interrupt process.
{
    // Ignore limit switches if already in an alarm state or in-process of executing an alarm.
    // When in the alarm state, Grbl should have been reset or will force a reset, so any pending
    // moves in the planner and serial buffers are all cleared and newly sent blocks will be
    // locked out until a homing cycle or a kill lock command. Allows the user to disable the hard
    // limit setting if their limits are constantly triggering after a reset and move their axes.
    if(sys.state != STATE_ALARM)
    {
        if(!(sys_rt_exec_alarm))
        {
            if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_FORCE_HARD_LIMIT_CHECK))
            {
                Delay_ms(2);
                uint8_t lim = Limits_GetState(true);

                // Check limit pin state.
                if(lim)
                {
                    MC_Reset(); // Initiate system kill.
                    System_SetExecAlarm(EXEC_ALARM_HARD_LIMIT); // Indicate hard limit critical event
                }
            }
            else
            {
                // Initiate system kill.
                MC_Reset();
                // Indicate hard limit critical event
                System_SetExecAlarm(EXEC_ALARM_HARD_LIMIT);
            }
        }
    }
}


// Homes the specified cycle axes, sets the machine position, and performs a pull-off motion after
// completing. Homing is a special motion case, which involves rapid uncontrolled stops to locate
// the trigger point of the limit switches. The rapid stops are handled by a system level axis lock
// mask, which prevents the stepper algorithm from executing step pulses. Homing motions typically
// circumvent the processes for executing motions in normal operation.
// NOTE: Only the abort realtime command can interrupt this process.
// TODO: Move limit pin-specific calls to a general function for portability.
void Limits_GoHome(uint8_t cycle_mask)
{
    if(sys.abort)
    {
        // Block if system reset has been issued.
        return;
    }

    // Initialize plan data struct for homing motion. Spindle and coolant are disabled.
    Planner_LineData_t plan_data;
    Planner_LineData_t *pl_data = &plan_data;
    memset(pl_data,0,sizeof(Planner_LineData_t));
    pl_data->condition = (PL_COND_FLAG_SYSTEM_MOTION|PL_COND_FLAG_NO_FEED_OVERRIDE);
    pl_data->line_number = HOMING_CYCLE_LINE_NUMBER;

    // Initialize variables used for homing computations.
    uint8_t n_cycle = (2*N_HOMING_LOCATE_CYCLE+1);
    uint8_t step_pin[N_AXIS];
    float target[N_AXIS];
    float max_travel = 0.0;
    uint8_t idx;
    for(idx = 0; idx < N_AXIS; idx++)
    {
        // Initialize step pin masks
        step_pin[idx] = Settings_GetStepPinMask(idx);

#ifdef COREXY
        if((idx == A_MOTOR) || (idx == B_MOTOR))
        {
            step_pin[idx] = (Settings_GetStepPinMask(X_AXIS) | Settings_GetStepPinMask(Y_AXIS));
        }
#endif

        if(BIT_IS_TRUE(cycle_mask, BIT(idx)))
        {
            // Set target based on max_travel setting. Ensure homing switches engaged with search scalar.
            // NOTE: settings.max_travel[] is stored as a negative value.
            max_travel = max(max_travel, (-HOMING_AXIS_SEARCH_SCALAR)*settings.max_travel[idx]);
        }
    }

    // Set search mode with approach at seek rate to quickly engage the specified cycle_mask limit switches.
    bool approach = true;
    float homing_rate = settings.homing_seek_rate;

    uint8_t limit_state, axislock, n_active_axis;
    do
    {
        System_ConvertArraySteps2Mpos(target,sys_position);

        // Initialize and declare variables needed for homing routine.
        axislock = 0;
        n_active_axis = 0;
        for(idx = 0; idx < N_AXIS; idx++)
        {
            // Set target location for active axes and setup computation for homing rate.
            if(BIT_IS_TRUE(cycle_mask,BIT(idx)))
            {
                n_active_axis++;
#ifdef COREXY
                if(idx == X_AXIS)
                {
                    int32_t axis_position = system_convert_corexy_to_y_axis_steps(sys_position);
                    sys_position[A_MOTOR] = axis_position;
                    sys_position[B_MOTOR] = -axis_position;
                }
                else if (idx == Y_AXIS)
                {
                    int32_t axis_position = system_convert_corexy_to_x_axis_steps(sys_position);
                    sys_position[A_MOTOR] = sys_position[B_MOTOR] = axis_position;
                }
                else
                {
                    sys_position[Z_AXIS] = 0;
                }
#else
                sys_position[idx] = 0;
#endif
                // Set target direction based on cycle mask and homing cycle approach state.
                // NOTE: This happens to compile smaller than any other implementation tried.
                if(BIT_IS_TRUE(settings.homing_dir_mask, BIT(idx)))
                {
                    if (approach)
                    {
                        target[idx] = -max_travel;
                    }
                    else
                    {
                        target[idx] = max_travel;
                    }
                }
                else
                {
                    if(approach)
                    {
                        target[idx] = max_travel;
                    }
                    else
                    {
                        target[idx] = -max_travel;
                    }
                }
                // Apply axislock to the step port pins active in this cycle.
                axislock |= step_pin[idx];
            }

        }

        // [sqrtf(N_AXIS)] Adjust so individual axes all move at homing rate.
        homing_rate *= sqrtf(n_active_axis);
        sys.homing_axis_lock = axislock;

        // Perform homing cycle. Planner buffer should be empty, as required to initiate the homing cycle.
        pl_data->feed_rate = homing_rate; // Set current homing rate.
        Planner_BufferLine(target, pl_data); // Bypass mc_line(). Directly plan homing motion.

        // Set to execute homing motion and clear existing flags.
        sys.step_control = STEP_CONTROL_EXECUTE_SYS_MOTION;
        // Prep and fill segment buffer from newly planned block.
        Stepper_PrepareBuffer();
        // Initiate motion
        Stepper_WakeUp();

        do
        {
            if(approach)
            {
                // Check limit state. Lock out cycle axes when they change.
                limit_state = Limits_GetState(true);
                for(idx = 0; idx < N_AXIS; idx++)
                {
                    if(axislock & step_pin[idx])
                    {
                        if(limit_state & (1 << idx))
                        {
#ifdef COREXY
                            if(idx == Z_AXIS)
                            {
                                axislock &= ~(step_pin[Z_AXIS]);
                            }
                            else
                            {
                                axislock &= ~(step_pin[A_MOTOR]|step_pin[B_MOTOR]);
                            }
#else
                            axislock &= ~(step_pin[idx]);
#endif
                        }
                    }
                }

                sys.homing_axis_lock = axislock;
            }

            // Check and prep segment buffer. NOTE: Should take no longer than 200us.
            Stepper_PrepareBuffer();

            // Exit routines: No time to run protocol_execute_realtime() in this loop.
            if(sys_rt_exec_state & (EXEC_SAFETY_DOOR | EXEC_RESET | EXEC_CYCLE_STOP))
            {
                uint16_t rt_exec = sys_rt_exec_state;

                // Homing failure condition: Reset issued during cycle.
                if(rt_exec & EXEC_RESET)
                {
                    System_SetExecAlarm(EXEC_ALARM_HOMING_FAIL_RESET);
                }
                // Homing failure condition: Safety door was opened.
                if(rt_exec & EXEC_SAFETY_DOOR)
                {
                    System_SetExecAlarm(EXEC_ALARM_HOMING_FAIL_DOOR);
                }
                // Homing failure condition: Limit switch still engaged after pull-off motion
                if(!approach && (Limits_GetState(true) & cycle_mask))
                {
                    System_SetExecAlarm(EXEC_ALARM_HOMING_FAIL_PULLOFF);
                }
                // Homing failure condition: Limit switch not found during approach.
                if(approach && (rt_exec & EXEC_CYCLE_STOP))
                {
                    System_SetExecAlarm(EXEC_ALARM_HOMING_FAIL_APPROACH);
                }
                if(sys_rt_exec_alarm)
                {
                    // Stop motors, if they are running.
                    MC_Reset();
                    Protocol_ExecuteRealtime();

                    return;
                }
                else
                {
                    // Pull-off motion complete. Disable CYCLE_STOP from executing.
                    System_ClearExecStateFlag(EXEC_CYCLE_STOP);
                    break;
                }
            }
            // TODO:
        }
        while(0x3F & axislock);

        // Immediately force kill steppers and reset step segment buffer.
        Stepper_Reset();
        // Delay to allow transient dynamics to dissipate.
        Delay_ms(settings.homing_debounce_delay);

        // Reverse direction and reset homing rate for locate cycle(s).
        approach = !approach;

        // After first cycle, homing enters locating phase. Shorten search to pull-off distance.
        if(approach)
        {
            max_travel = settings.homing_pulloff*HOMING_AXIS_LOCATE_SCALAR;
            homing_rate = settings.homing_feed_rate;
        }
        else
        {
            max_travel = settings.homing_pulloff;
            homing_rate = settings.homing_seek_rate;
        }

    }
    while(n_cycle-- > 0);

    // The active cycle axes should now be homed and machine limits have been located. By
    // default, Grbl defines machine space as all negative, as do most CNCs. Since limit switches
    // can be on either side of an axes, check and set axes machine zero appropriately. Also,
    // set up pull-off maneuver from axes limit switches that have been homed. This provides
    // some initial clearance off the switches and should also help prevent them from falsely
    // triggering when hard limits are enabled or when more than one axes shares a limit pin.
    int32_t set_axis_position = 0;
    // Set machine positions for homed limit switches. Don't update non-homed axes.
    for (idx = 0; idx < N_AXIS; idx++)
    {
        // NOTE: settings.max_travel[] is stored as a negative value.
        if (cycle_mask & BIT(idx))
        {
            if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_HOMING_FORCE_SET_ORIGIN))
            {
                set_axis_position = 0;
            }
            else
            {
                if (BIT_IS_TRUE(settings.homing_dir_mask, BIT(idx)))
                {
                    set_axis_position = lroundf((settings.max_travel[idx] + settings.homing_pulloff) * settings.steps_per_mm[idx]);
                }
                else
                {
                    set_axis_position = lroundf(-settings.homing_pulloff * settings.steps_per_mm[idx]);
                }
            }
#ifdef COREXY
            if(idx == X_AXIS)
            {
                int32_t off_axis_position = system_convert_corexy_to_y_axis_steps(sys_position);
                sys_position[A_MOTOR] = set_axis_position + off_axis_position;
                sys_position[B_MOTOR] = set_axis_position - off_axis_position;
            }
            else if(idx == Y_AXIS)
            {
                int32_t off_axis_position = system_convert_corexy_to_x_axis_steps(sys_position);
                sys_position[A_MOTOR] = off_axis_position + set_axis_position;
                sys_position[B_MOTOR] = off_axis_position - set_axis_position;
            }
            else
            {
                sys_position[idx] = set_axis_position;
            }
#else
            sys_position[idx] = set_axis_position;
#endif
        }
    }

    // Necessary for backlash compensation
    MC_Init();

    // Return step control to normal operation.
    sys.step_control = STEP_CONTROL_NORMAL_OP;
    // Machine is homed and knows its position
    sys.is_homed = 1;
}


// Performs a soft limit check. Called from mc_line() only. Assumes the machine has been homed,
// the workspace volume is in all negative space, and the system is in normal operation.
// NOTE: Used by jogging to limit travel within soft-limit volume.
void Limits_SoftCheck(const float *target)
{
    if(System_CheckTravelLimits(target))
    {
        sys.soft_limit = true;

        // Force feed hold if cycle is active. All buffered blocks are guaranteed to be within
        // workspace volume so just come to a controlled stop so position is not lost. When complete
        // enter alarm mode.
        if(sys.state == STATE_CYCLE)
        {
            System_SetExecStateFlag(EXEC_FEED_HOLD);
            do
            {
                Protocol_ExecuteRealtime();

                if(sys.abort)
                {
                    return;
                }
            }
            while(sys.state != STATE_IDLE);
        }

        // Issue system reset and ensure spindle and coolant are shutdown.
        MC_Reset();
        // Indicate soft limit critical event
        System_SetExecAlarm(EXEC_ALARM_SOFT_LIMIT);
        // Execute to enter critical event loop and system abort
        Protocol_ExecuteRealtime();
    }
}
