/*
  Settings.c - eeprom configuration handling
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
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
#include "Settings.h"
#include "CRC.h"
#include "Config.h"
#include "GCode.h"
#include "Limits.h"
#include "Nvm.h"
#include "Probe.h"
#include "Protocol.h"
#include "Report.h"
#include "SpindleControl.h"
#include "Stepper.h"
#include "System.h"
#include "defaults.h"
#include <string.h>


static void WriteGlobalSettings(void);
static uint8_t ReadGlobalSettings(void);


Settings_t settings;


// Initialize the config subsystem
void Settings_Init(void)
{
    Nvm_Init();

    if (!ReadGlobalSettings())
    {
        Report_StatusMessage(STATUS_SETTING_READ_FAIL);
        // Force restore all EEPROM data.
        Settings_Restore(SETTINGS_RESTORE_ALL);
        Report_GrblSettings();
    }

    // Read tool table
    TT_Init();
}


// Method to restore EEPROM-saved Grbl global settings back to defaults.
void Settings_Restore(const uint8_t restore_flag)
{
    sys.state = STATE_BUSY;
    Report_RealtimeStatus();

    if (restore_flag & SETTINGS_RESTORE_DEFAULTS)
    {
        settings.input_invert_mask = DEFAULT_SYSTEM_INVERT_MASK;
        settings.stepper_idle_lock_time = DEFAULT_STEPPER_IDLE_LOCK_TIME;
        settings.step_invert_mask = DEFAULT_STEPPING_INVERT_MASK;
        settings.dir_invert_mask = DEFAULT_DIRECTION_INVERT_MASK;
        settings.status_report_mask = DEFAULT_STATUS_REPORT_MASK;
        settings.junction_deviation = DEFAULT_JUNCTION_DEVIATION;
        settings.arc_tolerance = DEFAULT_ARC_TOLERANCE;

        settings.rpm_max = DEFAULT_SPINDLE_RPM_MAX;
        settings.rpm_min = DEFAULT_SPINDLE_RPM_MIN;
        settings.enc_ppr = DEFAULT_ENCODER_PULSES_PER_REV;

        settings.homing_dir_mask = DEFAULT_HOMING_DIR_MASK;
        settings.homing_feed_rate = DEFAULT_HOMING_FEED_RATE;
        settings.homing_seek_rate = DEFAULT_HOMING_SEEK_RATE;
        settings.homing_debounce_delay = DEFAULT_HOMING_DEBOUNCE_DELAY;
        settings.homing_pulloff = DEFAULT_HOMING_PULLOFF;

        // Flags
        settings.flags = 0;
        if (DEFAULT_REPORT_INCHES)
        {
            settings.flags |= BITFLAG_REPORT_INCHES;
        }
        if (DEFAULT_LASER_MODE)
        {
            settings.flags |= BITFLAG_LASER_MODE;
        }
        if (DEFAULT_INVERT_ST_ENABLE)
        {
            settings.flags |= BITFLAG_INVERT_ST_ENABLE;
        }
        if (DEFAULT_HARD_LIMIT_ENABLE)
        {
            settings.flags |= BITFLAG_HARD_LIMIT_ENABLE;
        }
        if (DEFAULT_HOMING_ENABLE)
        {
            settings.flags |= BITFLAG_HOMING_ENABLE;
        }
        if (DEFAULT_SOFT_LIMIT_ENABLE)
        {
            settings.flags |= BITFLAG_SOFT_LIMIT_ENABLE;
        }
        if (DEFAULT_INVERT_LIMIT_PINS)
        {
            settings.flags |= BITFLAG_INVERT_LIMIT_PINS;
        }
        if (DEFAULT_INVERT_PROBE_PIN)
        {
            settings.flags |= BITFLAG_INVERT_PROBE_PIN;
        }

        // flags_ext
        settings.flags_ext = 0;
        if (DEFAULT_LATHE_MODE)
        {
            settings.flags_ext |= BITFLAG_LATHE_MODE;
        }
        if (BUFFER_SYNC_DURING_EEPROM_WRITE)
        {
            settings.flags_ext |= BITFLAG_BUFFER_SYNC_NVM_WRITE;
        }
        if (DEFAULT_ENABLE_M7)
        {
            settings.flags_ext |= BITFLAG_ENABLE_M7;
        }
        if (HARD_LIMIT_FORCE_STATE_CHECK)
        {
            settings.flags_ext |= BITFLAG_FORCE_HARD_LIMIT_CHECK;
        }
        if (ENABLE_BACKLASH_COMPENSATION)
        {
            settings.flags_ext |= BITFLAG_ENABLE_BACKLASH_COMP;
        }
        if (USE_MULTI_AXIS)
        {
            settings.flags_ext |= BITFLAG_ENABLE_MULTI_AXIS;
        }
        if (HOMING_INIT_LOCK)
        {
            settings.flags_ext |= BITFLAG_HOMING_INIT_LOCK;
        }
        if (HOMING_FORCE_SET_ORIGIN)
        {
            settings.flags_ext |= BITFLAG_HOMING_FORCE_SET_ORIGIN;
        }
        if (FORCE_INITIALIZATION_ALARM)
        {
            settings.flags_ext |= BITFLAG_FORCE_INITIALIZATION_ALARM;
        }
        if (CHECK_LIMITS_AT_INIT)
        {
            settings.flags_ext |= BITFLAG_CHECK_LIMITS_AT_INIT;
        }

        // Flags report
        settings.flags_report = 0;
        if (DEFAULT_REPORT_FIELD_BUFFER_STATE)
        {
            settings.flags_report |= BITFLAG_REPORT_FIELD_BUFFER_STATE;
        }
        if (DEFAULT_REPORT_FIELD_PIN_STATE)
        {
            settings.flags_report |= BITFLAG_REPORT_FIELD_PIN_STATE;
        }
        if (DEFAULT_REPORT_FIELD_CURRENT_FEED_SPEED)
        {
            settings.flags_report |= BITFLAG_REPORT_FIELD_CUR_FEED_SPEED;
        }
        if (DEFAULT_REPORT_FIELD_WORK_COORD_OFFSET)
        {
            settings.flags_report |= BITFLAG_REPORT_FIELD_WORK_COORD_OFFSET;
        }
        if (DEFAULT_REPORT_FIELD_OVERRIDES)
        {
            settings.flags_report |= BITFLAG_REPORT_FIELD_OVERRIDES;
        }
        if (DEFAULT_REPORT_FIELD_LINE_NUMBERS)
        {
            settings.flags_report |= BITFLAG_REPORT_FIELD_LINE_NUMBERS;
        }

        settings.steps_per_mm[X_AXIS] = DEFAULT_X_STEPS_PER_MM;
        settings.steps_per_mm[Y_AXIS] = DEFAULT_Y_STEPS_PER_MM;
        settings.steps_per_mm[Z_AXIS] = DEFAULT_Z_STEPS_PER_MM;
        settings.steps_per_mm[A_AXIS] = DEFAULT_A_STEPS_PER_DEG;
        settings.steps_per_mm[B_AXIS] = DEFAULT_B_STEPS_PER_DEG;

        settings.max_rate[X_AXIS] = DEFAULT_X_MAX_RATE;
        settings.max_rate[Y_AXIS] = DEFAULT_Y_MAX_RATE;
        settings.max_rate[Z_AXIS] = DEFAULT_Z_MAX_RATE;
        settings.max_rate[A_AXIS] = DEFAULT_A_MAX_RATE;
        settings.max_rate[B_AXIS] = DEFAULT_B_MAX_RATE;

        settings.acceleration[X_AXIS] = DEFAULT_X_ACCELERATION;
        settings.acceleration[Y_AXIS] = DEFAULT_Y_ACCELERATION;
        settings.acceleration[Z_AXIS] = DEFAULT_Z_ACCELERATION;
        settings.acceleration[A_AXIS] = DEFAULT_A_ACCELERATION;
        settings.acceleration[B_AXIS] = DEFAULT_B_ACCELERATION;

        settings.max_travel[X_AXIS] = (-DEFAULT_X_MAX_TRAVEL);
        settings.max_travel[Y_AXIS] = (-DEFAULT_Y_MAX_TRAVEL);
        settings.max_travel[Z_AXIS] = (-DEFAULT_Z_MAX_TRAVEL);
        settings.max_travel[A_AXIS] = (-DEFAULT_A_MAX_TRAVEL);
        settings.max_travel[B_AXIS] = (-DEFAULT_B_MAX_TRAVEL);

        settings.backlash[X_AXIS] = DEFAULT_X_BACKLASH;
        settings.backlash[Y_AXIS] = DEFAULT_Y_BACKLASH;
        settings.backlash[Z_AXIS] = DEFAULT_Z_BACKLASH;

        settings.tool_change = DEFAULT_TOOL_CHANGE_MODE;
        settings.tls_valid = 0;
        settings.tls_position[X_AXIS] = 0;
        settings.tls_position[Y_AXIS] = 0;
        settings.tls_position[Z_AXIS] = 0;

        WriteGlobalSettings();
    }

    if (restore_flag & SETTINGS_RESTORE_PARAMETERS)
    {
        float coord_data[N_AXIS];

        memset(&coord_data, 0, sizeof(coord_data));

        for (uint8_t idx = 0; idx <= SETTING_INDEX_NCOORD; idx++)
        {
            Settings_WriteCoordData(idx, coord_data);
        }
        Nvm_Update();
    }

    if (restore_flag & SETTINGS_RESTORE_COORDS)
    {
        float coord_data[N_AXIS];

        memset(&coord_data, 0, sizeof(coord_data));

        for (uint8_t idx = 0; idx < N_COORDINATE_SYSTEM; idx++)
        {
            Settings_WriteCoordData(idx, coord_data);
        }
        Nvm_Update();
    }

    if (restore_flag & SETTINGS_RESTORE_STARTUP_LINES)
    {
        for (uint8_t i = 0; i < N_STARTUP_LINE; i++)
        {
            Nvm_WriteByte(EEPROM_ADDR_STARTUP_BLOCK + ((STARTUP_LINE_LEN + 1) * i), 0);
            Nvm_WriteByte(EEPROM_ADDR_STARTUP_BLOCK + ((STARTUP_LINE_LEN + 1) * i + 1), 0); // Checksum
        }

        Nvm_Update();
    }

    if (restore_flag & SETTINGS_RESTORE_BUILD_INFO)
    {
        Nvm_WriteByte(EEPROM_ADDR_BUILD_INFO, 0);
        Nvm_WriteByte(EEPROM_ADDR_BUILD_INFO + 1, 0); // Checksum
        Nvm_Update();
    }

    if (restore_flag & SETTINGS_RESTORE_TOOLS)
    {
        TT_Reset();
        Nvm_Update();
    }

    sys.state = STATE_IDLE;
}


// Method to store startup lines into EEPROM
void Settings_StoreStartupLine(uint8_t n, const char *line)
{
    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_BUFFER_SYNC_NVM_WRITE))
    {
        // A startup line may contain a motion and be executing.
        Protocol_BufferSynchronize();
    }

    uint32_t addr = n*(STARTUP_LINE_LEN+1)+EEPROM_ADDR_STARTUP_BLOCK;
    Nvm_Write(addr, (uint8_t*)line, STARTUP_LINE_LEN);
    Nvm_Update();
}


// Method to store build info into EEPROM
// NOTE: This function can only be called in IDLE state.
void Settings_StoreBuildInfo(const char *line)
{
    // Build info can only be stored when state is IDLE.
    Nvm_Write(EEPROM_ADDR_BUILD_INFO, (uint8_t*)line, STARTUP_LINE_LEN);
    Nvm_Update();
}


// Method to store coord data parameters into EEPROM
void Settings_WriteCoordData(uint8_t coord_select, const float *coord_data)
{
    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_BUFFER_SYNC_NVM_WRITE))
    {
        Protocol_BufferSynchronize();
    }

    uint32_t addr = coord_select*(sizeof(float)*N_AXIS+1) + EEPROM_ADDR_PARAMETERS;
    Nvm_Write(addr, (uint8_t*)coord_data, sizeof(float)*N_AXIS);

    uint8_t crc = CRC_CalculateCRC8((const uint8_t *)coord_data, sizeof(float) * N_AXIS);
    Nvm_WriteByte(addr + (sizeof(float) * N_AXIS), crc);

    Nvm_Update();
}


// Reads startup line from EEPROM. Updated pointed line string data.
uint8_t Settings_ReadStartupLine(uint8_t n, char *line)
{
    uint32_t addr = n*(STARTUP_LINE_LEN+1)+EEPROM_ADDR_STARTUP_BLOCK;
    if(!(Nvm_Read((uint8_t*)line, addr, STARTUP_LINE_LEN)))
    {
        // Reset line with default value
        line[0] = 0;
        Settings_StoreStartupLine(n, line);

        return false;
    }

    return true;
}


void Settings_StoreToolTable(const ToolTable_t *table)
{
    Nvm_Write(EEPROM_ADDR_TOOLTABLE, (uint8_t*)table, sizeof(ToolTable_t));

    uint8_t crc = CRC_CalculateCRC8((const uint8_t *)table, sizeof(ToolTable_t));
    Nvm_WriteByte(EEPROM_ADDR_TOOLTABLE_CRC, crc);
}


/*void Settings_StoreToolParams(uint8_t tool_nr, const ToolParams_t *params)
{
    Nvm_Write(EEPROM_ADDR_TOOLTABLE+(tool_nr*sizeof(ToolParams_t)), (uint8_t*)params, sizeof(ToolParams_t));

    uint8_t crc = CRC_CalculateCRC8((const uint8_t *)table, sizeof(ToolTable_t));
    Nvm_WriteByte(EEPROM_ADDR_TOOLTABLE_CRC, crc);
}*/


uint8_t Settings_ReadToolTable(ToolTable_t *table)
{
    if(!(Nvm_Read((uint8_t*)table, EEPROM_ADDR_TOOLTABLE, sizeof(ToolTable_t))))
    {
        return false;
    }
    uint8_t crc = CRC_CalculateCRC8((const uint8_t *)table, sizeof(ToolTable_t));
    if (crc != Nvm_ReadByte(EEPROM_ADDR_TOOLTABLE_CRC))
    {
        return false;
    }

    return true;
}


// Reads startup line from EEPROM. Updated pointed line string data.
uint8_t Settings_ReadBuildInfo(char *line)
{
    if(!(Nvm_Read((uint8_t*)line, EEPROM_ADDR_BUILD_INFO, STARTUP_LINE_LEN)))
    {
        // Reset line with default value
        line[0] = 0;
        Settings_StoreBuildInfo(line);

        return false;
    }

    return true;
}


// Read selected coordinate data from EEPROM. Updates pointed coord_data value.
uint8_t Settings_ReadCoordData(uint8_t coord_select, float *coord_data)
{
    uint32_t addr = coord_select*(sizeof(float)*N_AXIS+1) + EEPROM_ADDR_PARAMETERS;
    if(!(Nvm_Read((uint8_t*)coord_data, addr, sizeof(float)*N_AXIS)))
    {
        // Reset with default zero vector
        memset(coord_data, 0, sizeof(float) * N_AXIS);
        Settings_WriteCoordData(coord_select, coord_data);

        return false;
    }
    uint8_t crc = CRC_CalculateCRC8((const uint8_t *)coord_data, sizeof(float)*N_AXIS);
    if (crc != Nvm_ReadByte(addr + (sizeof(float) * N_AXIS)))
    {
        return false;
    }

    return true;
}


// A helper method to set settings from command line
uint8_t Settings_StoreGlobalSetting(uint8_t parameter, float value)
{
    if(value < 0.0)
    {
        return STATUS_NEGATIVE_VALUE;
    }

    if(parameter >= AXIS_SETTINGS_START_VAL)
    {
        // Store axis configuration. Axis numbering sequence set by AXIS_SETTING defines.
        // NOTE: Ensure the setting index corresponds to the report.c settings printout.
        parameter -= AXIS_SETTINGS_START_VAL;
        uint8_t set_idx = 0;

        while(set_idx < AXIS_N_SETTINGS)
        {
            if(parameter < N_AXIS)
            {
                // Valid axis setting found.
                switch (set_idx)
                {
                case 0:
#ifdef MAX_STEP_RATE_HZ
                    if (value*settings.max_rate[parameter] > (MAX_STEP_RATE_HZ*60.0))
                    {
                        return (STATUS_MAX_STEP_RATE_EXCEEDED);
                    }
#endif
                    settings.steps_per_mm[parameter] = value;
                    break;

                case 1:
#ifdef MAX_STEP_RATE_HZ
                    if (value*settings.steps_per_mm[parameter] > (MAX_STEP_RATE_HZ*60.0))
                    {
                        return (STATUS_MAX_STEP_RATE_EXCEEDED);
                    }
#endif
                    settings.max_rate[parameter] = value;
                    break;

                case 2:
                    // Convert to mm/min^2 for grbl internal use.
                    settings.acceleration[parameter] = value*60*60;
                    break;
                case 3:
                    // Store as negative for grbl internal use.
                    settings.max_travel[parameter] = -value;
                    break;
                case 4:
                    settings.backlash[parameter] = value;
                    break;
                }
                // Exit while-loop after setting has been configured and proceed to the EEPROM write call.
                break;
            }
            else
            {
                set_idx++;
                // If axis index greater than N_AXIS or setting index greater than number of axis settings, error out.
                if ((parameter < AXIS_SETTINGS_INCREMENT) || (set_idx == AXIS_N_SETTINGS))
                {
                    return (STATUS_INVALID_STATEMENT);
                }
                parameter -= AXIS_SETTINGS_INCREMENT;
            }
        }
    }
    else
    {
        // Store non-axis Grbl settings
        uint8_t int_value = truncf(value);

        switch(parameter)
        {
        case 0:
            settings.input_invert_mask = int_value & CONTROL_MASK;
            break;

        case 1:
            settings.stepper_idle_lock_time = int_value;
            break;

        case 2:
            settings.step_invert_mask = int_value;
            // Regenerate step and direction port invert masks.
            Stepper_GenerateStepDirInvertMasks();
            break;

        case 3:
            settings.dir_invert_mask = int_value;
            // Regenerate step and direction port invert masks.
            Stepper_GenerateStepDirInvertMasks();
            break;

        case 4: // Reset to ensure change. Immediate re-init may cause problems.
            if (int_value)
            {
                settings.flags |= BITFLAG_INVERT_ST_ENABLE;
            }
            else
            {
                settings.flags &= ~BITFLAG_INVERT_ST_ENABLE;
            }
            break;

        case 5: // Reset to ensure change. Immediate re-init may cause problems.
            if (int_value)
            {
                settings.flags |= BITFLAG_INVERT_LIMIT_PINS;
            }
            else
            {
                settings.flags &= ~BITFLAG_INVERT_LIMIT_PINS;
            }
            break;

        case 6: // Reset to ensure change. Immediate re-init may cause problems.
            if (int_value)
            {
                settings.flags |= BITFLAG_INVERT_PROBE_PIN;
            }
            else
            {
                settings.flags &= ~BITFLAG_INVERT_PROBE_PIN;
            }
            Probe_ConfigureInvertMask(false);
            break;

        case 7:
            settings.flags_report = int_value;
            break;

        case 10:
            settings.status_report_mask = int_value;
            break;

        case 11:
            settings.junction_deviation = value;
            break;

        case 12:
            settings.arc_tolerance = value;
            break;

        case 13:
            if (int_value)
            {
                settings.flags |= BITFLAG_REPORT_INCHES;
            }
            else
            {
                settings.flags &= ~BITFLAG_REPORT_INCHES;
            }
            // Make sure WCO is immediately updated.
            System_FlagWcoChange();
            break;

        case 14:
            // Check for range?
            settings.tool_change = int_value;
            break;

        case 15:
            settings.enc_ppr = (uint16_t)value;
            break;

        case 20:
            if (int_value)
            {
                if (BIT_IS_FALSE(settings.flags, BITFLAG_HOMING_ENABLE))
                {
                    return (STATUS_SOFT_LIMIT_ERROR);
                }
                settings.flags |= BITFLAG_SOFT_LIMIT_ENABLE;
            }
            else
            {
                settings.flags &= ~BITFLAG_SOFT_LIMIT_ENABLE;
            }
            break;

        case 21:
            if (int_value)
            {
                settings.flags |= BITFLAG_HARD_LIMIT_ENABLE;
            }
            else
            {
                settings.flags &= ~BITFLAG_HARD_LIMIT_ENABLE;
            }
            // Re-init to immediately change. NOTE: Nice to have but could be problematic later.
            Limits_Init();
            break;

        case 22:
            if (int_value)
            {
                settings.flags |= BITFLAG_HOMING_ENABLE;
            }
            else
            {
                settings.flags &= ~BITFLAG_HOMING_ENABLE;
                // Force disable soft-limits.
                settings.flags &= ~BITFLAG_SOFT_LIMIT_ENABLE;
            }
            break;

        case 23:
            settings.homing_dir_mask = int_value;
            break;

        case 24:
            settings.homing_feed_rate = value;
            break;

        case 25:
            settings.homing_seek_rate = value;
            break;

        case 26:
            settings.homing_debounce_delay = int_value;
            break;

        case 27:
            settings.homing_pulloff = value;
            break;

        case 30:
            settings.rpm_max = value;
            // Re-initialize spindle rpm calibration
            Spindle_Init();
            break;

        case 31:
            settings.rpm_min = value;
            // Re-initialize spindle rpm calibration
            Spindle_Init();
            break;

        case 32:
            if (int_value)
            {
                settings.flags |= BITFLAG_LASER_MODE;
            }
            else
            {
                settings.flags &= ~BITFLAG_LASER_MODE;
            }
            break;

        case 33:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_LATHE_MODE;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_LATHE_MODE;
            }
            break;

        case 34:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_BUFFER_SYNC_NVM_WRITE;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_BUFFER_SYNC_NVM_WRITE;
            }
            break;

        case 35:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_ENABLE_M7;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_ENABLE_M7;
            }
            break;

        case 36:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_FORCE_HARD_LIMIT_CHECK;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_FORCE_HARD_LIMIT_CHECK;
            }
            break;

        case 37:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_ENABLE_BACKLASH_COMP;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_ENABLE_BACKLASH_COMP;
            }
            break;

        case 38:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_ENABLE_MULTI_AXIS;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_ENABLE_MULTI_AXIS;
            }
            break;

        case 39:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_HOMING_INIT_LOCK;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_HOMING_INIT_LOCK;
            }
            break;

        case 40:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_HOMING_FORCE_SET_ORIGIN;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_HOMING_FORCE_SET_ORIGIN;
            }
            break;

        case 41:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_FORCE_INITIALIZATION_ALARM;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_FORCE_INITIALIZATION_ALARM;
            }
            break;

        case 42:
            if (int_value)
            {
                settings.flags_ext |= BITFLAG_CHECK_LIMITS_AT_INIT;
            }
            else
            {
                settings.flags_ext &= ~BITFLAG_CHECK_LIMITS_AT_INIT;
            }
            break;

        default:
            return (STATUS_INVALID_STATEMENT);
        }
    }

    WriteGlobalSettings();

    return 0;
}


void Settings_StoreTlsPosition(void)
{
    memcpy(settings.tls_position, sys_position, sizeof(float)*N_AXIS);
    settings.tls_valid = 1;

    WriteGlobalSettings();
}


// Returns step pin mask according to Grbl internal axis indexing.
uint8_t Settings_GetStepPinMask(uint8_t axis_idx)
{
    if(axis_idx == X_AXIS)
    {
        return (1<<X_STEP_BIT);
    }
    if(axis_idx == Y_AXIS)
    {
        return (1 << Y_STEP_BIT);
    }
    if (axis_idx == Z_AXIS)
    {
        return (1 << Z_STEP_BIT);
    }
    if (axis_idx == A_AXIS)
    {
        return (1 << A_STEP_BIT);
    }
    if (axis_idx == B_AXIS)
    {
        return (1 << B_STEP_BIT);
    }

    return 0;
}


// Returns direction pin mask according to Grbl internal axis indexing.
uint8_t Settings_GetDirectionPinMask(uint8_t axis_idx)
{
    if(axis_idx == X_AXIS)
    {
        return (1<<X_DIRECTION_BIT);
    }
    if(axis_idx == Y_AXIS)
    {
        return (1<<Y_DIRECTION_BIT);
    }
    if(axis_idx == Z_AXIS)
    {
        return (1<<Z_DIRECTION_BIT);
    }
    if(axis_idx == A_AXIS)
    {
        return (1<<A_DIRECTION_BIT);
    }
    if (axis_idx == B_AXIS)
    {
        return (1 << B_DIRECTION_BIT);
    }

    return 0;
}


// Returns limit pin mask according to Grbl internal axis indexing.
uint8_t Settings_GetLimitPinMask(uint8_t axis_idx)
{
    if(axis_idx == X_AXIS)
    {
        return (1<<X_STEP_BIT);
    }
    if(axis_idx == Y_AXIS)
    {
        return (1<<Y_STEP_BIT);
    }
    if(axis_idx == Z_AXIS)
    {
        return (1<<Z_STEP_BIT);
    }
    if(axis_idx == A_AXIS)
    {
        return (1<<A_STEP_BIT);
    }
    if (axis_idx == B_AXIS)
    {
        return (1 << B_STEP_BIT);
    }

    return 0;
}


// Method to store Grbl global settings struct and version number into EEPROM
// NOTE: This function can only be called in IDLE state.
static void WriteGlobalSettings(void)
{
    Nvm_WriteByte(0, SETTINGS_VERSION);
    Nvm_Write(EEPROM_ADDR_GLOBAL, (uint8_t *)&settings, sizeof(Settings_t));

    uint8_t crc = CRC_CalculateCRC8((const uint8_t *)&settings, sizeof(settings));
    Nvm_WriteByte(EEPROM_ADDR_GLOBAL_CRC, crc);

    Nvm_Update();
}


// Reads Grbl global settings struct from EEPROM.
static uint8_t ReadGlobalSettings(void)
{
    // Check version-byte of eeprom
    uint8_t version = Nvm_ReadByte(EEPROM_ADDR_VERSION);

    if (version == SETTINGS_VERSION)
    {
        // Read settings-record and check checksum
        if (!(Nvm_Read((uint8_t *)&settings, EEPROM_ADDR_GLOBAL, sizeof(Settings_t))))
        {
            return false;
        }
        uint8_t crc = CRC_CalculateCRC8((const uint8_t *)&settings, sizeof(settings));
        if (crc != Nvm_ReadByte(EEPROM_ADDR_GLOBAL_CRC))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}
