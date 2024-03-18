/*
  System.c - Handles system level commands and real-time processes
  Part of Grbl-Advanced

  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2017-2024 Patrick F.

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Config.h"
#include "GCode.h"
#include "GPIO.h"
#include "MotionControl.h"
#include "Protocol.h"
#include "Report.h"
#include "Settings.h"
#include "Stepper.h"
#include "System.h"
#include "ToolChange.h"
#include "System32.h"


// Declare system global variable structure
System_t sys;
// Real-time machine (aka home) position vector in steps.
int32_t sys_position[N_AXIS];
// Last probe position in machine coordinates and steps.
int32_t sys_probe_position[N_AXIS];
// Probing state value.  Used to coordinate the probing cycle with stepper ISR.
volatile uint8_t sys_probe_state;
// Global realtime executor bitflag variable for state management. See EXEC bitmasks.
volatile uint16_t sys_rt_exec_state;
// Global realtime executor bitflag variable for setting various alarms.
volatile uint8_t sys_rt_exec_alarm;
// Global realtime executor bitflag variable for motion-based overrides.
volatile uint8_t sys_rt_exec_motion_override;
// Global realtime executor bitflag variable for spindle/coolant overrides.
volatile uint8_t sys_rt_exec_accessory_override;

//static uint8_t initial_state = 0;
static uint8_t last_state = 0;


void System_Init(void)
{
    GPIO_InitGPIO(GPIO_SYSTEM);
    last_state = 0;

    sys.system_flags |= BITFLAG_ENABLE_SYSTEM_INPUT;
    System_GetControlState(false);
}


void System_Clear(void)
{
    // Clear system struct variable.
    memset(&sys, 0, sizeof(System_t));

    // Set overrides to 100%
    sys.f_override = DEFAULT_FEED_OVERRIDE;
    sys.r_override = DEFAULT_RAPID_OVERRIDE;
    sys.spindle_speed_ovr = DEFAULT_SPINDLE_SPEED_OVERRIDE;

    sys.system_flags |= BITFLAG_ENABLE_SYSTEM_INPUT;
}


void System_ResetPosition(void)
{
    // Clear machine position.
    memset(sys_position, 0 , sizeof(sys_position));
}


// Returns control pin state as a uint8 bitfield. Each bit indicates the input pin state, where
// triggered is 1 and not triggered is 0. Invert mask is applied. Bitfield organization is
// defined by the CONTROL_PIN_INDEX in the header file.
uint8_t System_GetControlState(bool held)
{
    uint8_t control_state = 0;
    uint8_t pin = ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)<<CONTROL_RESET_BIT) |
                   (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)<<CONTROL_FEED_HOLD_BIT) |
                   (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4)<<CONTROL_CYCLE_START_BIT) |
                   (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)<<CONTROL_SAFETY_DOOR_BIT));

    // Invert control pins if necessary
    pin ^= CONTROL_MASK & settings.input_invert_mask;

    // XOR: Only get changed
    uint8_t tmp_state = pin ^ last_state;
    // Only get active
    tmp_state &= pin;

    if(held)
    {
        // Get current state of inputs
        tmp_state = pin;
    }

    if (tmp_state)
    {
        if (BIT_IS_TRUE(tmp_state, (1 << CONTROL_RESET_BIT)))
        {
            control_state |= CONTROL_PIN_INDEX_RESET;
        }
        if (BIT_IS_TRUE(tmp_state, (1 << CONTROL_FEED_HOLD_BIT)))
        {
            control_state |= CONTROL_PIN_INDEX_FEED_HOLD;
        }
        if (BIT_IS_TRUE(tmp_state, (1 << CONTROL_CYCLE_START_BIT)))
        {
            control_state |= CONTROL_PIN_INDEX_CYCLE_START;
        }
        /*if(BIT_IS_TRUE(pin, (1<<CONTROL_SAFETY_DOOR_BIT)))
        {
            control_state |= CONTROL_PIN_INDEX_SAFETY_DOOR;
        }*/
    }

    last_state = pin;

    return control_state;
}


// Pin change interrupt for pin-out commands, i.e. cycle start, feed hold, and reset. Sets
// only the realtime command execute variable to have the main program execute these when
// its ready. This works exactly like the character-based realtime commands when picked off
// directly from the incoming serial data stream.
void System_PinChangeISR(void)
{
    uint8_t pin = System_GetControlState(true);

    if(pin)
    {
        if(BIT_IS_TRUE(pin, CONTROL_PIN_INDEX_RESET))
        {
            MC_Reset();
        }
        else if(BIT_IS_TRUE(pin, CONTROL_PIN_INDEX_CYCLE_START))
        {
            BIT_TRUE(sys_rt_exec_state, EXEC_CYCLE_START);
        }
        if(BIT_IS_TRUE(pin, CONTROL_PIN_INDEX_FEED_HOLD))
        {
            BIT_TRUE(sys_rt_exec_state, EXEC_FEED_HOLD);
        }
        if(BIT_IS_TRUE(pin, CONTROL_PIN_INDEX_SAFETY_DOOR))
        {
            BIT_TRUE(sys_rt_exec_state, EXEC_SAFETY_DOOR);
        }
    }
}


// Returns if safety door is ajar(T) or closed(F), based on pin state.
uint8_t System_CheckSafetyDoorAjar(void)
{
    return (System_GetControlState(true) & CONTROL_PIN_INDEX_SAFETY_DOOR);
}


// Executes user startup script, if stored.
void System_ExecuteStartup(char *line)
{
    for (uint8_t n = 0; n < N_STARTUP_LINE; n++)
    {
        if(!(Settings_ReadStartupLine(n, line)))
        {
            line[0] = 0;
            Report_ExecuteStartupMessage(line, STATUS_SETTING_READ_FAIL);
        }
        else
        {
            if(line[0] != 0)
            {
                uint8_t status_code = GC_ExecuteLine(line);

                Report_ExecuteStartupMessage(line,status_code);
            }
        }
    }
}


// Directs and executes one line of formatted input from protocol_process. While mostly
// incoming streaming g-code blocks, this also executes Grbl internal commands, such as
// settings, initiating the homing cycle, and toggling switch states. This differs from
// the realtime command module by being susceptible to when Grbl is ready to execute the
// next line during a cycle, so for switches like block delete, the switch only effects
// the lines that are processed afterward, not necessarily real-time during a cycle,
// since there are motions already stored in the buffer. However, this 'lag' should not
// be an issue, since these commands are not typically used during a cycle.
uint8_t System_ExecuteLine(char *line)
{
    uint8_t char_counter = 1;
    uint8_t helper_var = 0; // Helper variable
    float parameter, value;

    switch(line[char_counter])
    {
    case 0:
        Report_GrblHelp();
        break;

    case 'J': // Jogging
        // Execute only if in IDLE or JOG states.
        if(sys.state != STATE_IDLE && sys.state != STATE_JOG)
        {
            return STATUS_IDLE_ERROR;
        }
        if(line[2] != '=')
        {
            return STATUS_INVALID_STATEMENT;
        }
        // NOTE: $J= is ignored inside g-code parser and used to detect jog motions.
        return GC_ExecuteLine(line);
        break;

    case '$':
    case 'G':
    case 'C':
    case 'X':
        if(line[2] != 0)
        {
            return (STATUS_INVALID_STATEMENT);
        }

        switch(line[1])
        {
        case '$': // Prints Grbl settings
            if(sys.state & (STATE_CYCLE | STATE_HOLD))
            {
                // Block during cycle. Takes too long to print.
                return (STATUS_IDLE_ERROR);
            }
            else
            {
                Report_GrblSettings();
            }
            break;

        case 'G': // Prints gcode parser state
            // TODO: Move this to realtime commands for GUIs to request this data during suspend-state.
            Report_GCodeModes();
            break;

        case 'C': // Set check g-code mode [IDLE/CHECK]
            // Perform reset when toggling off. Check g-code mode should only work if Grbl
            // is idle and ready, regardless of alarm locks. This is mainly to keep things
            // simple and consistent.
            if(sys.state == STATE_CHECK_MODE)
            {
                MC_Reset();
                Report_FeedbackMessage(MESSAGE_DISABLED);
            }
            else
            {
                if(sys.state)
                {
                    // Requires no alarm mode.
                    return STATUS_IDLE_ERROR;
                }

                sys.state = STATE_CHECK_MODE;
                Report_FeedbackMessage(MESSAGE_ENABLED);
            }
            break;

        case 'X': // Disable alarm lock [ALARM]
            if(BIT_IS_TRUE(sys.state, STATE_ALARM))
            {
                // Block if safety door is ajar.
                if(System_CheckSafetyDoorAjar())
                {
                    return (STATUS_CHECK_DOOR);
                }
                if (System_GetControlState(true))
                {
                    return (STATUS_CHECK_INPUT);
                }

                Report_FeedbackMessage(MESSAGE_ALARM_UNLOCK);
                sys.state = STATE_IDLE;
                Stepper_WakeUp();
                // Don't run startup script. Prevents stored moves in startup from causing accidents.
            }
            // Otherwise, no effect.
            break;
        }
        break;

    case 'T':
        if(line[++char_counter] == 0)
        {
            // Tool change by user finished. Continue execution
            System_ClearExecStateFlag(EXEC_TOOL_CHANGE);
            sys.state = STATE_IDLE;

            // Check if machine is homed
            if(sys.is_homed)
            {
                // Change tool with probing
                if(settings.tool_change == 2)
                {
                    // Check if TLS is valid
                    if(settings.tls_valid)
                    {
                        // Probe new tool
                        if(TC_ProbeTLS() != 0)
                        {
                            return STATUS_PROBE_ERROR;
                        }
                    }
                    else
                    {
                        return STATUS_TLS_NOT_SET;
                    }
                }
                else if(settings.tool_change == 3)
                {
                    // Change tool with tool table (Apply offsets)
                    TC_ApplyToolOffset();
                }
                else
                {
                    return STATUS_SETTING_DISABLED;
                }
            }
            else
            {
                return STATUS_MACHINE_NOT_HOMED;
            }
        }
        else
        {
            // Print tool params
            char c;
            ToolParams_t params = {};
            char num[4] = {};
            uint8_t idx = 0;

            do
            {
                c = line[char_counter++];
                num[idx++] = c;
            }
            while(isdigit(c) && idx < 3);
            num[idx] = '\0';

            if(c == '=')
            {
                // Save params of new tool
                char tmp_float[10] = {};
                int t = 0;
                float value_f[4] = {};

                // Read floats [x.x:x.x:x.x:x.x]
                for (uint8_t i = 0; i < 4; i++)
                {
                    t = ExtractFloat(&line[char_counter], t, tmp_float);

                    // Check if float was found
                    if(strlen(tmp_float) > 0)
                    {
                        // Convert string to float
                        //sscanf(tmp_float, "%f", &value_f[i]);
                        //tmp_float[0] = '\0';
                        uint8_t cnt = 0;
                        Read_Float(tmp_float, &cnt, &value_f[i]);
                        tmp_float[0] = '\0';
                    }
                    else
                    {
                        // Couldn't find a float value
                        break;
                    }
                }

                params.x_offset = value_f[0];
                params.y_offset = value_f[1];
                params.z_offset = value_f[2];
                params.reserved = value_f[3];

                // Store tool params
                TT_SaveToolParams(atoi(num), &params);
            }
            else
            {
                Report_ToolParams(atoi(num));
            }
        }
        break;

    case 'P':
        if(sys.is_homed)
        {
            // Store position of TLS
            Settings_StoreTlsPosition();
        }
        else
        {
            return STATUS_MACHINE_NOT_HOMED;
        }
        break;

    default:
        // Block any system command that requires the state as IDLE/ALARM. (i.e. EEPROM, homing)
        if(!(sys.state == STATE_IDLE || sys.state == STATE_ALARM))
        {
            return (STATUS_IDLE_ERROR);
        }

        switch(line[1])
        {
        case '#': // Print Grbl NGC parameters
            if(line[2] != 0)
            {
                return STATUS_INVALID_STATEMENT;
            }
            else
            {
                Report_NgcParams();
            }
            break;

        case 'H': // Perform homing cycle [IDLE/ALARM]
            if(BIT_IS_FALSE(settings.flags, BITFLAG_HOMING_ENABLE))
            {
                return (STATUS_SETTING_DISABLED);
            }
            if(System_CheckSafetyDoorAjar())
            {
                // Block if safety door is ajar.
                return STATUS_CHECK_DOOR;
            }

            // Set homing state
            sys.state = STATE_HOMING;

            if(line[2] == 0)
            {
                MC_HomigCycle(HOMING_CYCLE_ALL);
#ifdef HOMING_SINGLE_AXIS_COMMANDS
            }
            else if(line[3] == 0)
            {
                switch(line[2])
                {
                case 'X':
                    MC_HomigCycle(HOMING_CYCLE_X);
                    break;

                case 'Y':
                    MC_HomigCycle(HOMING_CYCLE_Y);
                    break;

                case 'Z':
                    MC_HomigCycle(HOMING_CYCLE_Z);
                    break;

                case 'A':
                    MC_HomigCycle(HOMING_CYCLE_A);
                    break;

                case 'B':
                    MC_HomigCycle(HOMING_CYCLE_B);
                    break;

                default:
                    return STATUS_INVALID_STATEMENT;
                }
#endif
            }
            else
            {
                return STATUS_INVALID_STATEMENT;
            }

            if(!sys.abort)
            {
                // Execute startup scripts after successful homing.
                // Set to IDLE when complete.
                sys.state = STATE_IDLE;
                // Set steppers to the settings idle state before returning.
                Stepper_Disable(0);

                if(line[2] == 0)
                {
                    System_ExecuteStartup(line);
                }
            }
            break;

        case 'S': // Puts Grbl to sleep [IDLE/ALARM]
            if((line[2] != 'L') || (line[3] != 'P') || (line[4] != 0))
            {
                return (STATUS_INVALID_STATEMENT);
            }
            System_SetExecStateFlag(EXEC_SLEEP); // Set to execute sleep mode immediately
            break;

        case 'I': // Print or store build info. [IDLE/ALARM]
            if(line[++char_counter] == 0)
            {
                Settings_ReadBuildInfo(line);
                Report_BuildInfo(line);
#ifdef ENABLE_BUILD_INFO_WRITE_COMMAND
            }
            else   // Store startup line [IDLE/ALARM]
            {
                if(line[char_counter++] != '=')
                {
                    return STATUS_INVALID_STATEMENT;
                }
                // Set helper variable as counter to start of user info line.
                helper_var = char_counter;

                do
                {
                    line[char_counter-helper_var] = line[char_counter];
                }
                while(line[char_counter++] != 0);

                Settings_StoreBuildInfo(line);
#endif
            }
            break;

        case 'R': // Restore defaults [IDLE/ALARM]
            if((line[2] != 'S') || (line[3] != 'T') || (line[4] != '=') || (line[6] != 0))
            {
                return (STATUS_INVALID_STATEMENT);
            }

            switch(line[5])
            {
#ifdef ENABLE_RESTORE_EEPROM_DEFAULT_SETTINGS
            case '$':
                // Restore defaults
                Settings_Restore(SETTINGS_RESTORE_DEFAULTS);
                break;
#endif
#ifdef ENABLE_RESTORE_EEPROM_CLEAR_PARAMETERS
            case '#':
                // Reset coord system
                Settings_Restore(SETTINGS_RESTORE_PARAMETERS);
                break;
#endif
#ifdef ENABLE_RESTORE_EEPROM_WIPE_ALL
            case '*':
                // Restore all
                Settings_Restore(SETTINGS_RESTORE_ALL);
                break;
#endif
#ifdef ENABLE_RESTORE_EEPROM_CLEAR_TOOLS
            case 'T':
                // Reset tool table
                TT_Reset();
                break;
#endif
#ifdef ENABLE_RESTORE_EEPROM_CLEAR_COORD
            case 'C':
                // Reset coord system G54-G59
                Settings_Restore(SETTINGS_RESTORE_COORDS);
                break;
#endif
#ifdef ENABLE_RESTORE_EEPROM_CLEAR_STARTUP
            case 'N':
            {
                // Reset line with default value
                // Empty line
                char startup[STARTUP_LINE_LEN] = {};

                for (int i = 0; i < N_STARTUP_LINE; i++)
                {
                    Settings_StoreStartupLine(i, startup);
                }
                break;
            }
#endif
            default:
                return STATUS_INVALID_STATEMENT;
            }

            Report_FeedbackMessage(MESSAGE_RESTORE_DEFAULTS);
            // Force reset to ensure settings are initialized correctly.
            MC_Reset();
            break;

        case 'N': // Startup lines. [IDLE/ALARM]
            // Print startup lines
            if(line[++char_counter] == 0)
            {
                for(helper_var = 0; helper_var < N_STARTUP_LINE; helper_var++)
                {
                    if (!(Settings_ReadStartupLine(helper_var, line)))
                    {
                        Report_StatusMessage(STATUS_SETTING_READ_FAIL);
                    }
                    else
                    {
                        Report_StartupLine(helper_var,line);
                    }
                }
                break;
            }
            else   // Store startup line [IDLE Only] Prevents motion during ALARM.
            {
                if(sys.state != STATE_IDLE)
                {
                    // Store only when idle.
                    return STATUS_IDLE_ERROR;
                }
                // Set helper_var to flag storing method.
                helper_var = true;
                // No break. Continues into default: to read remaining command characters.
            }

        default:  // Storing setting methods [IDLE/ALARM]
            if(!Read_Float(line, &char_counter, &parameter))
            {
                return(STATUS_BAD_NUMBER_FORMAT);
            }
            if(line[char_counter++] != '=')
            {
                return (STATUS_INVALID_STATEMENT);
            }
            if(helper_var)   // Store startup line
            {
                // Prepare sending gcode block to gcode parser by shifting all characters
                helper_var = char_counter; // Set helper variable as counter to start of gcode block

                do
                {
                    line[char_counter-helper_var] = line[char_counter];
                }
                while(line[char_counter++] != 0);

                // Execute gcode block to ensure block is valid.
                // Set helper_var to returned status code.
                helper_var = GC_ExecuteLine(line);

                if(helper_var)
                {
                    return (helper_var);
                }
                else
                {
                    // Set helper_var to int value of parameter
                    helper_var = truncf(parameter);
                    if (helper_var < N_STARTUP_LINE)
                    {
                        Settings_StoreStartupLine(helper_var, line);
                    }
                }
            }
            else   // Store global setting.
            {
                if(!Read_Float(line, &char_counter, &value))
                {
                    return STATUS_BAD_NUMBER_FORMAT;
                }
                if((line[char_counter] != 0) || (parameter > 255))
                {
                    return STATUS_INVALID_STATEMENT;
                }

                return Settings_StoreGlobalSetting((uint8_t)parameter, value);
            }
        }
    }
    // If '$' command makes it to here, then everything's ok.
    return STATUS_OK;
}


void System_FlagWcoChange(void)
{
#ifdef FORCE_BUFFER_SYNC_DURING_WCO_CHANGE
    Protocol_BufferSynchronize();
#endif
    sys.report_wco_counter = 0;
}


// Returns machine position of axis 'idx'. Must be sent a 'step' array.
// NOTE: If motor steps and machine position are not in the same coordinate frame, this function
//   serves as a central place to compute the transformation.
float System_ConvertAxisSteps2Mpos(const int32_t *steps, const uint8_t idx)
{
    float pos = 0.0;

    if (steps)
    {
#ifdef COREXY
        if (idx == X_AXIS)
        {
            pos = (float)system_convert_corexy_to_x_axis_steps(steps) / settings.steps_per_mm[idx];
        }
        else if (idx == Y_AXIS)
        {
            pos = (float)system_convert_corexy_to_y_axis_steps(steps) / settings.steps_per_mm[idx];
        }
        else
        {
            pos = steps[idx] / settings.steps_per_mm[idx];
        }
#else
        if (settings.steps_per_mm[idx] > 0.0)
        {
            pos = steps[idx] / settings.steps_per_mm[idx];
        }
#endif
    }

    return pos;
}


void System_ConvertArraySteps2Mpos(float *position, const int32_t *steps)
{
    if (position)
    {
        for (uint8_t idx = 0; idx < N_AXIS; idx++)
        {
            position[idx] = System_ConvertAxisSteps2Mpos(steps, idx);
        }
    }

    return;
}


// CoreXY calculation only. Returns x or y-axis "steps" based on CoreXY motor steps.
#ifdef COREXY
int32_t system_convert_corexy_to_x_axis_steps(const int32_t *steps)
{
    return ((steps[A_MOTOR] + steps[B_MOTOR])/2);
}

int32_t system_convert_corexy_to_y_axis_steps(const int32_t *steps)
{
    return ((steps[A_MOTOR] - steps[B_MOTOR])/2);
}
#endif


// Checks and reports if target array exceeds machine travel limits.
uint8_t System_CheckTravelLimits(const float *target)
{
    for (uint8_t idx = 0; idx < N_AXIS; idx++)
    {
        if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_HOMING_FORCE_SET_ORIGIN))
        {
            // When homing forced set origin is enabled, soft limits checks need to account for directionality.
            // NOTE: max_travel is stored as negative
            if (BIT_IS_TRUE(settings.homing_dir_mask, BIT(idx)))
            {
                if (target[idx] < 0 || target[idx] > -settings.max_travel[idx])
                {
                    return true;
                }
            }
            else
            {
                if (target[idx] > 0 || target[idx] < settings.max_travel[idx])
                {
                    return true;
                }
            }
        }
        else
        {
            // NOTE: max_travel is stored as negative
            if (target[idx] > 0 || target[idx] < settings.max_travel[idx])
            {
                return true;
            }
        }
    }

    return false;
}


// Special handlers for setting and clearing Grbl's real-time execution flags.
inline void System_SetExecStateFlag(uint16_t mask)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_state |= (mask);

    __set_PRIMASK(primask);
}


inline void System_ClearExecStateFlag(uint16_t mask)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_state &= ~(mask);

    __set_PRIMASK(primask);
}


inline void System_SetExecAlarm(uint8_t code)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_alarm = code;

    __set_PRIMASK(primask);
}


inline void System_ClearExecAlarm(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_alarm = 0;

    __set_PRIMASK(primask);
}


inline void System_SetExecMotionOverrideFlag(uint8_t mask)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_motion_override |= (mask);

    __set_PRIMASK(primask);
}


inline void System_SetExecAccessoryOverrideFlag(uint8_t mask)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_accessory_override |= (mask);

    __set_PRIMASK(primask);
}


inline void System_ClearExecMotionOverride(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_motion_override = 0;

    __set_PRIMASK(primask);
}


inline void System_ClearExecAccessoryOverrides(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    sys_rt_exec_accessory_override = 0;

    __set_PRIMASK(primask);
}
