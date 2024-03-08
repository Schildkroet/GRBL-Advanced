/*
  Settings.h - Configuration handling
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "util.h"
#include "ToolTable.h"


// Version of the EEPROM data. Will be used to migrate existing data from older versions of Grbl
// when firmware is upgraded. Always stored in byte 0 of eeprom
#define SETTINGS_VERSION                        8  // NOTE: Check settings_reset() when moving to next version.


// Define bit flag masks for the boolean settings in settings.system_flags
#define BITFLAG_INVERT_RESET_PIN                BIT(0)
#define BITFLAG_INVERT_FEED_PIN                 BIT(1)
#define BITFLAG_INVERT_CYCLE_PIN                BIT(2)
#define BITFLAG_INVERT_SAFETY_PIN               BIT(3)
#define BITFLAG_ENABLE_LIMITS                   BIT(4)

// Define bit flag masks for the boolean settings in settings.flags.
#define BITFLAG_REPORT_INCHES                   BIT(0)
#define BITFLAG_LASER_MODE                      BIT(1)
#define BITFLAG_INVERT_ST_ENABLE                BIT(2)
#define BITFLAG_HARD_LIMIT_ENABLE               BIT(3)
#define BITFLAG_HOMING_ENABLE                   BIT(4)
#define BITFLAG_SOFT_LIMIT_ENABLE               BIT(5)
#define BITFLAG_INVERT_LIMIT_PINS               BIT(6)
#define BITFLAG_INVERT_PROBE_PIN                BIT(7)

// Define bit flag masks for the boolean settings in settings.flags_ext.
#define BITFLAG_LATHE_MODE                      BIT(0)
#define BITFLAG_BUFFER_SYNC_NVM_WRITE           BIT(1)
#define BITFLAG_ENABLE_M7                       BIT(2)
#define BITFLAG_FORCE_HARD_LIMIT_CHECK          BIT(3)
#define BITFLAG_ENABLE_BACKLASH_COMP            BIT(4)
#define BITFLAG_ENABLE_MULTI_AXIS               BIT(5)
#define BITFLAG_HOMING_INIT_LOCK                BIT(6)
#define BITFLAG_HOMING_FORCE_SET_ORIGIN         BIT(7)
#define BITFLAG_FORCE_INITIALIZATION_ALARM      BIT(8)
#define BITFLAG_CHECK_LIMITS_AT_INIT            BIT(9)

// Define bit flag masks for the boolean settings in settings.flags_report.
#define BITFLAG_REPORT_FIELD_BUFFER_STATE       BIT(0)
#define BITFLAG_REPORT_FIELD_PIN_STATE          BIT(1)
#define BITFLAG_REPORT_FIELD_CUR_FEED_SPEED     BIT(2)
#define BITFLAG_REPORT_FIELD_WORK_COORD_OFFSET  BIT(3)
#define BITFLAG_REPORT_FIELD_OVERRIDES          BIT(4)
#define BITFLAG_REPORT_FIELD_LINE_NUMBERS       BIT(5)

// Define status reporting boolean enable bit flags in settings.status_report_mask
#define BITFLAG_RT_STATUS_POSITION_TYPE         BIT(0)
#define BITFLAG_RT_STATUS_BUFFER_STATE          BIT(1)

// Define settings restore bitflags.
#define SETTINGS_RESTORE_DEFAULTS               BIT(0)
#define SETTINGS_RESTORE_PARAMETERS             BIT(1)
#define SETTINGS_RESTORE_STARTUP_LINES          BIT(2)
#define SETTINGS_RESTORE_BUILD_INFO             BIT(3)
#define SETTINGS_RESTORE_TOOLS                  BIT(4)
#define SETTINGS_RESTORE_COORDS                 BIT(5)

// Define EEPROM memory address location values for Grbl settings and parameters
// NOTE: Default 1KB EEPROM. The upper half is reserved for parameters and
// the startup script. The lower half contains the global settings and space for future
// developments.
#define EEPROM_ADDR_GLOBAL                  1U      // +163
#define EEPROM_ADDR_TOOLTABLE               180U    // +320
#define EEPROM_ADDR_PARAMETERS              512U    // +160
#define EEPROM_ADDR_STARTUP_BLOCK           768U    // +150
#define EEPROM_ADDR_BUILD_INFO              926U    // +80

// CRC addresses
#define EEPROM_ADDR_GLOBAL_CRC              1018U
#define EEPROM_ADDR_TOOLTABLE_CRC           1019U

// Define EEPROM address indexing for coordinate parameters
#define N_COORDINATE_SYSTEM                 6  // Number of supported work coordinate systems (from index 1)
#define SETTING_INDEX_NCOORD                N_COORDINATE_SYSTEM+1 // Total number of system stored (from index 0)

// NOTE: Work coordinate indices are (0=G54, 1=G55, ... , 6=G59)
#define SETTING_INDEX_G28                   N_COORDINATE_SYSTEM    // Home position 1
#define SETTING_INDEX_G30                   N_COORDINATE_SYSTEM+1  // Home position 2
// #define SETTING_INDEX_G92    N_COORDINATE_SYSTEM+2  // Coordinate offset (G92.2,G92.3 not supported)

// Define Grbl axis settings numbering scheme. Starts at START_VAL, every INCREMENT, over N_SETTINGS.
#define AXIS_N_SETTINGS                     5   // Number of settings per axis
#define AXIS_SETTINGS_START_VAL             100 // NOTE: Reserving settings values >= 100 for axis settings. Up to 255.
#define AXIS_SETTINGS_INCREMENT             10  // Must be greater than the number of axis settings

#ifndef SETTINGS_RESTORE_ALL
  #define SETTINGS_RESTORE_ALL              0xFF // All bitflags
#endif


#pragma pack(push, 1) // exact fit - no padding
// Global persistent settings (Stored from byte EEPROM_ADDR_GLOBAL onwards); 111 Bytes
typedef struct
{
    // Axis settings
    float steps_per_mm[N_AXIS];
    float max_rate[N_AXIS];
    float acceleration[N_AXIS];
    float max_travel[N_AXIS];

    float backlash[N_AXIS];

    // Tool change mode
    uint8_t tool_change;

    // Position of tool length sensor (only XYZ axis)
    int32_t tls_position[N_AXIS];
    uint8_t tls_valid;

    // Remaining Grbl settings
    uint8_t system_flags;
    uint8_t step_invert_mask;
    uint8_t dir_invert_mask;
    uint8_t stepper_idle_lock_time;     // If max value 255, steppers do not disable.
    uint8_t status_report_mask;         // Mask to indicate desired report data.
    float junction_deviation;
    float arc_tolerance;

    float rpm_max;
    float rpm_min;
    uint16_t enc_ppr;   // Encoder pulses per revolution

    uint8_t flags;  // Contains default boolean settings
    uint16_t flags_ext;
    uint8_t flags_report;

    uint8_t homing_dir_mask;
    float homing_feed_rate;
    float homing_seek_rate;
    uint16_t homing_debounce_delay;
    float homing_pulloff;
} Settings_t;
#pragma pack(pop)


extern Settings_t settings;


// Initialize the configuration subsystem (load settings from EEPROM)
void Settings_Init(void);

// Helper function to clear and restore EEPROM defaults
void Settings_Restore(uint8_t restore_flag);

// A helper method to set new settings from command line
uint8_t Settings_StoreGlobalSetting(uint8_t parameter, float value);

// Save current position to tls position
void Settings_StoreTlsPosition(void);

// Stores the protocol line variable as a startup line in EEPROM
void Settings_StoreStartupLine(uint8_t n, const char *line);

// Stores tool table in EEPROM
void Settings_StoreToolTable(const ToolTable_t *table);

void Settings_StoreToolParams(uint8_t tool_nr, const ToolParams_t *params);

// Read tool table
uint8_t Settings_ReadToolTable(ToolTable_t *table);

// Reads an EEPROM startup line to the protocol line variable
uint8_t Settings_ReadStartupLine(uint8_t n, char *line);

// Stores build info user-defined string
void Settings_StoreBuildInfo(const char *line);

// Reads build info user-defined string
uint8_t Settings_ReadBuildInfo(char *line);

// Writes selected coordinate data to EEPROM
void Settings_WriteCoordData(uint8_t coord_select, const float *coord_data);

// Reads selected coordinate data from EEPROM
uint8_t Settings_ReadCoordData(uint8_t coord_select, float *coord_data);

// Returns the step pin mask according to Grbl's internal axis numbering
uint8_t Settings_GetStepPinMask(uint8_t i);

// Returns the direction pin mask according to Grbl's internal axis numbering
uint8_t Settings_GetDirectionPinMask(uint8_t i);

// Returns the limit pin mask according to Grbl's internal axis numbering
uint8_t Settings_GetLimitPinMask(uint8_t i);


#endif // SETTINGS_H
