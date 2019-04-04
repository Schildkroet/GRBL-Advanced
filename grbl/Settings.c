/*
  Settings.c - eeprom configuration handling
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
#include "Config.h"
#include "GCode.h"
#include "Limits.h"
#include "Protocol.h"
#include "Probe.h"
#include "Report.h"
#include "Settings.h"
#include "SpindleControl.h"
#include "System.h"
#include "Stepper.h"
#include "defaults.h"
#include "Nvm.h"
#include <stdint.h>
#include <string.h>


Settings_t settings;


// Method to store startup lines into EEPROM
void Settings_StoreStartupLine(uint8_t n, char *line)
{
#ifdef FORCE_BUFFER_SYNC_DURING_EEPROM_WRITE
	Protocol_BufferSynchronize(); // A startup line may contain a motion and be executing.
#endif

	uint32_t addr = n*(STARTUP_LINE_LEN+1)+EEPROM_ADDR_STARTUP_BLOCK;
	Nvm_Write(addr, (uint8_t*)line, STARTUP_LINE_LEN);
	Nvm_Update();
}


// Method to store build info into EEPROM
// NOTE: This function can only be called in IDLE state.
void Settings_StoreBuildInfo(char *line)
{
	// Build info can only be stored when state is IDLE.
	Nvm_Write(EEPROM_ADDR_BUILD_INFO, (uint8_t*)line, STARTUP_LINE_LEN);
	Nvm_Update();
}


// Method to store coord data parameters into EEPROM
void Settings_WriteCoordData(uint8_t coord_select, float *coord_data)
{
#ifdef FORCE_BUFFER_SYNC_DURING_EEPROM_WRITE
    Protocol_BufferSynchronize();
#endif

	uint32_t addr = coord_select*(sizeof(float)*N_AXIS+1) + EEPROM_ADDR_PARAMETERS;
	Nvm_Write(addr, (uint8_t*)coord_data, sizeof(float)*N_AXIS);

	Nvm_Update();
}


// Method to store Grbl global settings struct and version number into EEPROM
// NOTE: This function can only be called in IDLE state.
void WriteGlobalSettings(void)
{
	Nvm_WriteByte(0, SETTINGS_VERSION);
	Nvm_Write(EEPROM_ADDR_GLOBAL, (uint8_t*)&settings, sizeof(Settings_t));

	Nvm_Update();
}


// Method to restore EEPROM-saved Grbl global settings back to defaults.
void Settings_Restore(uint8_t restore_flag) {
	if(restore_flag & SETTINGS_RESTORE_DEFAULTS) {
		settings.system_flags = DEFAULT_SYSTEM_INVERT_MASK;
		settings.stepper_idle_lock_time = DEFAULT_STEPPER_IDLE_LOCK_TIME;
		settings.step_invert_mask = DEFAULT_STEPPING_INVERT_MASK;
		settings.dir_invert_mask = DEFAULT_DIRECTION_INVERT_MASK;
		settings.status_report_mask = DEFAULT_STATUS_REPORT_MASK;
		settings.junction_deviation = DEFAULT_JUNCTION_DEVIATION;
		settings.arc_tolerance = DEFAULT_ARC_TOLERANCE;

		settings.rpm_max = DEFAULT_SPINDLE_RPM_MAX;
		settings.rpm_min = DEFAULT_SPINDLE_RPM_MIN;

		settings.homing_dir_mask = DEFAULT_HOMING_DIR_MASK;
		settings.homing_feed_rate = DEFAULT_HOMING_FEED_RATE;
		settings.homing_seek_rate = DEFAULT_HOMING_SEEK_RATE;
		settings.homing_debounce_delay = DEFAULT_HOMING_DEBOUNCE_DELAY;
		settings.homing_pulloff = DEFAULT_HOMING_PULLOFF;

		settings.flags = 0;
		if(DEFAULT_REPORT_INCHES) { settings.flags |= BITFLAG_REPORT_INCHES; }
		if(DEFAULT_LASER_MODE) { settings.flags |= BITFLAG_LASER_MODE; }
		if(DEFAULT_INVERT_ST_ENABLE) { settings.flags |= BITFLAG_INVERT_ST_ENABLE; }
		if(DEFAULT_HARD_LIMIT_ENABLE) { settings.flags |= BITFLAG_HARD_LIMIT_ENABLE; }
		if(DEFAULT_HOMING_ENABLE) { settings.flags |= BITFLAG_HOMING_ENABLE; }
		if(DEFAULT_SOFT_LIMIT_ENABLE) { settings.flags |= BITFLAG_SOFT_LIMIT_ENABLE; }
		if(DEFAULT_INVERT_LIMIT_PINS) { settings.flags |= BITFLAG_INVERT_LIMIT_PINS; }
		if(DEFAULT_INVERT_PROBE_PIN) { settings.flags |= BITFLAG_INVERT_PROBE_PIN; }

		settings.steps_per_mm[X_AXIS] = DEFAULT_X_STEPS_PER_MM;
		settings.steps_per_mm[Y_AXIS] = DEFAULT_Y_STEPS_PER_MM;
		settings.steps_per_mm[Z_AXIS] = DEFAULT_Z_STEPS_PER_MM;
		settings.max_rate[X_AXIS] = DEFAULT_X_MAX_RATE;
		settings.max_rate[Y_AXIS] = DEFAULT_Y_MAX_RATE;
		settings.max_rate[Z_AXIS] = DEFAULT_Z_MAX_RATE;
		settings.acceleration[X_AXIS] = DEFAULT_X_ACCELERATION;
		settings.acceleration[Y_AXIS] = DEFAULT_Y_ACCELERATION;
		settings.acceleration[Z_AXIS] = DEFAULT_Z_ACCELERATION;
		settings.max_travel[X_AXIS] = (-DEFAULT_X_MAX_TRAVEL);
		settings.max_travel[Y_AXIS] = (-DEFAULT_Y_MAX_TRAVEL);
		settings.max_travel[Z_AXIS] = (-DEFAULT_Z_MAX_TRAVEL);

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

	if(restore_flag & SETTINGS_RESTORE_PARAMETERS) {
		uint8_t idx;
		float coord_data[N_AXIS];

		memset(&coord_data, 0, sizeof(coord_data));

		for(idx = 0; idx <= SETTING_INDEX_NCOORD; idx++) {
			Settings_WriteCoordData(idx, coord_data);
		}
	}

	if(restore_flag & SETTINGS_RESTORE_STARTUP_LINES) {
#if N_STARTUP_LINE > 0
		Nvm_WriteByte(EEPROM_ADDR_STARTUP_BLOCK, 0);
		Nvm_WriteByte(EEPROM_ADDR_STARTUP_BLOCK+1, 0); // Checksum
#endif
#if N_STARTUP_LINE > 1
		Nvm_WriteByte(EEPROM_ADDR_STARTUP_BLOCK+(STARTUP_LINE_LEN+1), 0);
		Nvm_WriteByte(EEPROM_ADDR_STARTUP_BLOCK+(STARTUP_LINE_LEN+2), 0); // Checksum
#endif
		Nvm_Update();
	}

	if(restore_flag & SETTINGS_RESTORE_BUILD_INFO) {
		Nvm_WriteByte(EEPROM_ADDR_BUILD_INFO , 0);
		Nvm_WriteByte(EEPROM_ADDR_BUILD_INFO+1 , 0); // Checksum
		Nvm_Update();
	}
}


// Reads startup line from EEPROM. Updated pointed line string data.
uint8_t Settings_ReadStartupLine(uint8_t n, char *line)
{
	uint32_t addr = n*(STARTUP_LINE_LEN+1)+EEPROM_ADDR_STARTUP_BLOCK;
	if (!(Nvm_Read((uint8_t*)line, addr, STARTUP_LINE_LEN))) {
		// Reset line with default value
		line[0] = 0; // Empty line
		Settings_StoreStartupLine(n, line);

		return false;
	}

	return true;
}


// Reads startup line from EEPROM. Updated pointed line string data.
uint8_t Settings_ReadBuildInfo(char *line)
{
	if(!(Nvm_Read((uint8_t*)line, EEPROM_ADDR_BUILD_INFO, STARTUP_LINE_LEN))) {
		// Reset line with default value
		line[0] = 0; // Empty line
		Settings_StoreBuildInfo(line);

		return false;
	}

	return true;
}


// Read selected coordinate data from EEPROM. Updates pointed coord_data value.
uint8_t Settings_ReadCoordData(uint8_t coord_select, float *coord_data)
{
	uint32_t addr = coord_select*(sizeof(float)*N_AXIS+1) + EEPROM_ADDR_PARAMETERS;
	if(!(Nvm_Read((uint8_t*)coord_data, addr, sizeof(float)*N_AXIS))) {
		// Reset with default zero vector
		memset(&coord_data, 0.0, sizeof(coord_data));
		Settings_WriteCoordData(coord_select, coord_data);

		return false;
	}

	return true;
}


// Reads Grbl global settings struct from EEPROM.
uint8_t ReadGlobalSettings() {
	// Check version-byte of eeprom
	uint8_t version = Nvm_ReadByte(0);

	if(version == SETTINGS_VERSION) {
		// Read settings-record and check checksum
		if(!(Nvm_Read((uint8_t*)&settings, EEPROM_ADDR_GLOBAL, sizeof(Settings_t)))) {
			return false;
		}
	}
	else {
		return false;
	}

	return true;
}


// A helper method to set settings from command line
uint8_t Settings_StoreGlobalSetting(uint8_t parameter, float value) {
	if(value < 0.0) {
		return STATUS_NEGATIVE_VALUE;
	}

	if(parameter >= AXIS_SETTINGS_START_VAL) {
		// Store axis configuration. Axis numbering sequence set by AXIS_SETTING defines.
		// NOTE: Ensure the setting index corresponds to the report.c settings printout.
		parameter -= AXIS_SETTINGS_START_VAL;
		uint8_t set_idx = 0;

		while(set_idx < AXIS_N_SETTINGS) {
			if(parameter < N_AXIS) {
				// Valid axis setting found.
				switch (set_idx) {
					case 0:
#ifdef MAX_STEP_RATE_HZ
						if (value*settings.max_rate[parameter] > (MAX_STEP_RATE_HZ*60.0)) { return(STATUS_MAX_STEP_RATE_EXCEEDED); }
#endif
					settings.steps_per_mm[parameter] = value;
					break;

					case 1:
#ifdef MAX_STEP_RATE_HZ
						if (value*settings.steps_per_mm[parameter] > (MAX_STEP_RATE_HZ*60.0)) {  return(STATUS_MAX_STEP_RATE_EXCEEDED); }
#endif
					settings.max_rate[parameter] = value;
					break;

					case 2: settings.acceleration[parameter] = value*60*60; break; // Convert to mm/min^2 for grbl internal use.
					case 3: settings.max_travel[parameter] = -value; break;  // Store as negative for grbl internal use.
					case 4: settings.backlash[parameter] = value; break;
				}
				break; // Exit while-loop after setting has been configured and proceed to the EEPROM write call.
			}
			else {
				set_idx++;
				// If axis index greater than N_AXIS or setting index greater than number of axis settings, error out.
				if ((parameter < AXIS_SETTINGS_INCREMENT) || (set_idx == AXIS_N_SETTINGS)) { return(STATUS_INVALID_STATEMENT); }
				parameter -= AXIS_SETTINGS_INCREMENT;
			}
		}
	}
	else {
		// Store non-axis Grbl settings
		uint8_t int_value = trunc(value);

		switch(parameter)
		{
		case 0:
			settings.system_flags = int_value;
			break;

		case 1:
			settings.stepper_idle_lock_time = int_value;
			break;

		case 2:
			settings.step_invert_mask = int_value;
			Stepper_GenerateStepDirInvertMasks(); // Regenerate step and direction port invert masks.
			break;

		case 3:
			settings.dir_invert_mask = int_value;
			Stepper_GenerateStepDirInvertMasks(); // Regenerate step and direction port invert masks.
			break;

		case 4: // Reset to ensure change. Immediate re-init may cause problems.
			if (int_value) { settings.flags |= BITFLAG_INVERT_ST_ENABLE; }
			else { settings.flags &= ~BITFLAG_INVERT_ST_ENABLE; }
			break;

		case 5: // Reset to ensure change. Immediate re-init may cause problems.
			if (int_value) { settings.flags |= BITFLAG_INVERT_LIMIT_PINS; }
			else { settings.flags &= ~BITFLAG_INVERT_LIMIT_PINS; }
			break;

		case 6: // Reset to ensure change. Immediate re-init may cause problems.
			if (int_value) { settings.flags |= BITFLAG_INVERT_PROBE_PIN; }
			else { settings.flags &= ~BITFLAG_INVERT_PROBE_PIN; }
			Probe_ConfigureInvertMask(false);
			break;

		case 10: settings.status_report_mask = int_value; break;
		case 11: settings.junction_deviation = value; break;
		case 12: settings.arc_tolerance = value; break;
		case 13:
			if (int_value) { settings.flags |= BITFLAG_REPORT_INCHES; }
			else { settings.flags &= ~BITFLAG_REPORT_INCHES; }
			System_FlagWcoChange(); // Make sure WCO is immediately updated.
			break;

        case 14: settings.tool_change = int_value; break;   // Check for range?

		case 20:
			if (int_value) {
				if (BIT_IS_FALSE(settings.flags, BITFLAG_HOMING_ENABLE)) { return(STATUS_SOFT_LIMIT_ERROR); }
				settings.flags |= BITFLAG_SOFT_LIMIT_ENABLE;
			} else { settings.flags &= ~BITFLAG_SOFT_LIMIT_ENABLE; }
			break;

		case 21:
			if (int_value) { settings.flags |= BITFLAG_HARD_LIMIT_ENABLE; }
			else { settings.flags &= ~BITFLAG_HARD_LIMIT_ENABLE; }
			Limits_Init(); // Re-init to immediately change. NOTE: Nice to have but could be problematic later.
			break;

		case 22:
			if (int_value) { settings.flags |= BITFLAG_HOMING_ENABLE; }
			else {
				settings.flags &= ~BITFLAG_HOMING_ENABLE;
				settings.flags &= ~BITFLAG_SOFT_LIMIT_ENABLE; // Force disable soft-limits.
			}
			break;

		case 23: settings.homing_dir_mask = int_value; break;
		case 24: settings.homing_feed_rate = value; break;
		case 25: settings.homing_seek_rate = value; break;
		case 26: settings.homing_debounce_delay = int_value; break;
		case 27: settings.homing_pulloff = value; break;
		case 30: settings.rpm_max = value; Spindle_Init(); break; // Re-initialize spindle rpm calibration
		case 31: settings.rpm_min = value; Spindle_Init(); break; // Re-initialize spindle rpm calibration
		case 32:
				if (int_value) { settings.flags |= BITFLAG_LASER_MODE; }
				else { settings.flags &= ~BITFLAG_LASER_MODE; }
			break;

		default:
			return(STATUS_INVALID_STATEMENT);
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


// Initialize the config subsystem
void Settings_Init(void)
{
	Nvm_Init();

	if(!ReadGlobalSettings()) {
		Report_StatusMessage(STATUS_SETTING_READ_FAIL);
		Settings_Restore(SETTINGS_RESTORE_ALL); // Force restore all EEPROM data.
		Report_GrblSettings();
	}
}


// Returns step pin mask according to Grbl internal axis indexing.
uint8_t Settings_GetStepPinMask(uint8_t axis_idx)
{
	if(axis_idx == X_AXIS) { return (1<<X_STEP_BIT); }
	if(axis_idx == Y_AXIS) { return (1<<Y_STEP_BIT); }

	return (1<<Z_STEP_BIT);
}


// Returns direction pin mask according to Grbl internal axis indexing.
uint8_t Settings_GetDirectionPinMask(uint8_t axis_idx)
{
	if(axis_idx == X_AXIS) { return (1<<X_DIRECTION_BIT); }
	if(axis_idx == Y_AXIS) { return (1<<Y_DIRECTION_BIT); }

	return (1<<Z_DIRECTION_BIT);
}


// Returns limit pin mask according to Grbl internal axis indexing.
uint8_t Settings_GetLimitPinMask(uint8_t axis_idx)
{
	if(axis_idx == X_AXIS) { return (1<<X_STEP_BIT); }
	if(axis_idx == Y_AXIS) { return (1<<Y_STEP_BIT); }

	return (1<<Z_STEP_BIT);
}
