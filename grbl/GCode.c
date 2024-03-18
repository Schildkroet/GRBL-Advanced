/*
  GCode.c - rs274/ngc parser.
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c) 2018-2024 Patrick F.

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
#include <math.h>
#include "System.h"
#include "Settings.h"
#include "Jog.h"
#include "Report.h"
#include "Config.h"
#include "SpindleControl.h"
#include "CoolantControl.h"
#include "MotionControl.h"
#include "Protocol.h"
#include "SpindleControl.h"
#include "util.h"
#include "ToolChange.h"
#include "GCode.h"
#include "Print.h"


// Modal Group M9: Override control
#ifdef DEACTIVATE_PARKING_UPON_INIT
  #define OVERRIDE_DISABLED                 0 // (Default: Must be zero)
  #define OVERRIDE_PARKING_MOTION           1 // M56
#else
  #define OVERRIDE_PARKING_MOTION           0 // M56 (Default: Must be zero)
  #define OVERRIDE_DISABLED                 1 // Parking disabled.
#endif


// NOTE: Max line number is defined by the g-code standard to be 99999. It seems to be an
// arbitrary value, and some GUIs may require more. So we increased it based on a max safe
// value when converting a float (7.2 digit precision)s to an integer.
#define MAX_LINE_NUMBER                     10000000
#define MAX_TOOL_NUMBER                     255 // Limited by max unsigned 8-bit value

#define AXIS_COMMAND_NONE                   0
#define AXIS_COMMAND_NON_MODAL              1
#define AXIS_COMMAND_MOTION_MODE            2
#define AXIS_COMMAND_TOOL_LENGTH_OFFSET     3 // *Undefined but required


// Declare gc extern struct
Parser_State_t gc_state;
static Parser_Block_t gc_block;


void GC_Init(void)
{
    memset(&gc_state, 0, sizeof(Parser_State_t));

    for (uint8_t i = 0; i < N_AXIS; i++)
    {
        gc_state.tool_length_offset_dynamic[i] = 0.0;
    }

    // Load default G54 coordinate system.
    if(!(Settings_ReadCoordData(gc_state.modal.coord_select, gc_state.coord_system)))
    {
        Report_StatusMessage(STATUS_SETTING_READ_FAIL);
    }
}


// Sets g-code parser position in mm. Input in steps. Called by the system abort and hard
// limit pull-off routines.
void GC_SyncPosition(void)
{
    System_ConvertArraySteps2Mpos(gc_state.position, sys_position);
}


// Executes one line of 0-terminated G-Code. The line is assumed to contain only uppercase
// characters and signed floating point values (no whitespace). Comments and block delete
// characters have been removed. In this function, all units and positions are converted and
// exported to grbl's internal functions in terms of (mm, mm/min) and absolute machine
// coordinates, respectively.
uint8_t GC_ExecuteLine(const char *line)
{
    /* -------------------------------------------------------------------------------------
     STEP 1: Initialize parser block struct and copy current g-code state modes. The parser
     updates these modes and commands as the block line is parser and will only be used and
     executed after successful error-checking. The parser block struct also contains a block
     values struct, word tracking variables, and a non-modal commands tracker for the new
     block. This struct contains all of the necessary information to execute the block. */

    uint8_t axis_command = AXIS_COMMAND_NONE;
    uint8_t axis_0, axis_1, axis_linear;
    // Tracks G10 P coordinate selection for execution
    uint8_t coord_select = 0;

    // Initialize bitflag tracking variables for axis indices compatible operations.

    // XYZ tracking
    uint8_t axis_words = 0;
    // IJK tracking
    uint8_t ijk_words = 0;

    // Initialize command and value words and parser flags variables.

    // Tracks G and M command words. Also used for modal group violations.
    uint16_t command_words = 0;
    // Tracks value words.
    uint16_t value_words = 0;
    uint8_t gc_parser_flags = GC_PARSER_NONE;


    // Initialize the parser block struct.
    memset(&gc_block, 0, sizeof(Parser_Block_t));
    // Copy current modes
    memcpy(&gc_block.modal, &gc_state.modal, sizeof(GC_Modal_t));

    // Determine if the line is a jogging motion or a normal g-code block.
    // NOTE: `$J=` already parsed when passed to this function.
    if(line[0] == '$')
    {
        // Set G1 and G94 enforced modes to ensure accurate error checks.
        gc_parser_flags |= GC_PARSER_JOG_MOTION;
        gc_block.modal.motion = MOTION_MODE_LINEAR;
        gc_block.modal.feed_rate = FEED_RATE_MODE_UNITS_PER_MIN;
        // Initialize default line number reported during jog.
        gc_block.values.n = JOG_LINE_NUMBER;
    }

    /* -------------------------------------------------------------------------------------
     STEP 2: Import all g-code words in the block line. A g-code word is a letter followed by
     a number, which can either be a 'G'/'M' command or sets/assigns a command value. Also,
     perform initial error-checks for command word modal group violations, for any repeated
     words, and for negative values set for the value words F, N, P, T, and S. */

    // Bit-value for assigning tracking variables
    uint16_t word_bit = 0;
    uint8_t char_counter = 0;
    char letter = 0;
    float value = 0.0;
    uint16_t int_value = 0;
    uint16_t mantissa = 0;
    float old_xyz[N_AXIS] = {};
    uint8_t change_tool = 0, update_tooltable = 0, apply_tool = 0;
    uint8_t io_cmd = 0;


    memcpy(old_xyz, gc_state.position, N_AXIS*sizeof(float));

    if(gc_parser_flags & GC_PARSER_JOG_MOTION)
    {
        // Start parsing after `$J=`
        char_counter = 3;
    }
    else
    {
        char_counter = 0;
    }

    // Loop until no more g-code words in line.
    while(line[char_counter] != 0)
    {
        // Import the next g-code word, expecting a letter followed by a value. Otherwise, error out.
        letter = line[char_counter];
        if((letter < 'A') || (letter > 'Z'))
        {
            // [Expected word letter]
            return STATUS_EXPECTED_COMMAND_LETTER;
        }

        char_counter++;
        if(!Read_Float(line, &char_counter, &value))
        {
            // [Expected word value]
            return STATUS_BAD_NUMBER_FORMAT;
        }

        // Convert values to smaller uint8 significand and mantissa values for parsing this word.
        // NOTE: Mantissa is multiplied by 100 to catch non-integer command values. This is more
        // accurate than the NIST gcode requirement of x10 when used for commands, but not quite
        // accurate enough for value words that require integers to within 0.0001. This should be
        // a good enough comprimise and catch most all non-integer errors. To make it compliant,
        // we would simply need to change the mantissa to int16, but this add compiled flash space.
        // Maybe update this later.
        int_value = truncf(value);
        // Compute mantissa for Gxx.x commands.
        mantissa =  roundf(100*(value - int_value));
        // NOTE: Rounding must be used to catch small floating point errors.

        // Check if the g-code word is supported or errors due to modal group violations or has
        // been repeated in the g-code block. If ok, update the command or record its value.
        switch(letter)
        {
            /* 'G' and 'M' Command Words: Parse commands and check for modal group violations.
             NOTE: Modal group numbers are defined in Table 4 of NIST RS274-NGC v3, pg.20 */

        case 'G':
            // Determine 'G' command and its modal group
            switch(int_value)
            {
            case 7:
                // Lathe Diameter Mode
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE))
                {
                    word_bit = MODAL_GROUP_G12;
                    gc_block.modal.lathe_mode = LATHE_DIAMETER_MODE;
                }
                else
                {
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                break;

            case 8:
                // Lathe Radius Mode (default)
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE))
                {
                    word_bit = MODAL_GROUP_G12;
                    gc_block.modal.lathe_mode = LATHE_RADIUS_MODE;
                }
                else
                {
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                break;

            case 10:
            case 28:
            case 30:
            case 92:
                // Check for G10/28/30/92 being called with G0/1/2/3/38 on same block.
                // * G43.1 is also an axis command but is not explicitly defined this way.
                if(mantissa == 0) // Ignore G28.1, G30.1, and G92.1
                {
                    if (axis_command)
                    {
                        // [Axis word/command conflict]
                        return STATUS_GCODE_AXIS_COMMAND_CONFLICT;
                    }
                    axis_command = AXIS_COMMAND_NON_MODAL;
                }
                // No break. Continues to next line.

            case 4:
            case 53:
                word_bit = MODAL_GROUP_G0;
                gc_block.non_modal_command = int_value;

                if((int_value == 28) || (int_value == 30) || (int_value == 92))
                {
                    if(!((mantissa == 0) || (mantissa == 10)))
                    {
                        return STATUS_GCODE_UNSUPPORTED_COMMAND;
                    }
                    gc_block.non_modal_command += mantissa;
                    // Set to zero to indicate valid non-integer G command.
                    mantissa = 0;
                }
                break;

            case 33:
                // Spindle Synchronized Motion
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE))
                {
                    word_bit = MODAL_GROUP_G1;
                    gc_block.modal.motion = int_value;
                    axis_command = AXIS_COMMAND_MOTION_MODE;
                }
                else
                {
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                break;

            case 76:
                // Threading Cycle
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE))
                {
                    word_bit = MODAL_GROUP_G1;
                    gc_block.modal.motion = int_value;
                    axis_command = AXIS_COMMAND_MOTION_MODE;
                }
                else
                {
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                break;

            case 96:
                // Constant Surface Speed
                if (BIT_IS_TRUE(settings.flags_ext , BITFLAG_LATHE_MODE))
                {
                    word_bit = MODAL_GROUP_G14;
                    gc_block.modal.spindle_mode = SPINDLE_SURFACE_MODE;
                }
                else
                {
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                break;

            case 97:
                // RPM Mode (default)
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE))
                {
                    word_bit = MODAL_GROUP_G14;
                    gc_block.modal.spindle_mode = SPINDLE_RPM_MODE;
                }
                else
                {
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                break;

            case 0:
            case 1:
            case 2:
            case 3:
            case 38:
                // Check for G0/1/2/3/38 being called with G10/28/30/92 on same block.
                // * G43.1 is also an axis command but is not explicitly defined this way.
                if(axis_command)
                {
                    // [Axis word/command conflict]
                    return STATUS_GCODE_AXIS_COMMAND_CONFLICT;
                }
                axis_command = AXIS_COMMAND_MOTION_MODE;
                // No break. Continues to next line.

            case 80:
                word_bit = MODAL_GROUP_G1;
                gc_block.modal.motion = int_value;

                if(int_value == 38)
                {
                    if(!((mantissa == 20) || (mantissa == 30) || (mantissa == 40) || (mantissa == 50)))
                    {
                        // [Unsupported G38.x command]
                        return STATUS_GCODE_UNSUPPORTED_COMMAND;
                    }
                    gc_block.modal.motion += (mantissa/10)+100;
                    // Set to zero to indicate valid non-integer G command.
                    mantissa = 0;
                }
                break;

            case 73:
            case 81:
            case 82:
            case 83:
                // Canned drilling cycles
                word_bit = MODAL_GROUP_G1;
                gc_block.modal.motion = int_value;
                axis_command = AXIS_COMMAND_MOTION_MODE;
                break;

            case 98:
            case 99:
                // Set retract mode
                word_bit = MODAL_GROUP_G10;
                gc_block.modal.retract = int_value - 98;
                break;

            case 17:
            case 18:
            case 19:
                word_bit = MODAL_GROUP_G2;
                gc_block.modal.plane_select = int_value - 17;
                break;

            case 90:
            case 91:
                if(mantissa == 0)
                {
                    word_bit = MODAL_GROUP_G3;
                    gc_block.modal.distance = int_value - 90;
                }
                else
                {
                    word_bit = MODAL_GROUP_G4;
                    if ((mantissa != 10) || (int_value == 90))
                    {
                        // [G90.1 not supported]
                        return STATUS_GCODE_UNSUPPORTED_COMMAND;
                    }
                    // Set to zero to indicate valid non-integer G command.
                    mantissa = 0;
                    // Otherwise, arc IJK incremental mode is default. G91.1 does nothing.
                }
                break;

            case 93:
            case 94:
                word_bit = MODAL_GROUP_G5;
                gc_block.modal.feed_rate = 94 - int_value;
                break;

            case 20:
            case 21:
                word_bit = MODAL_GROUP_G6;
                gc_block.modal.units = 21 - int_value;
                break;

            case 40:
                word_bit = MODAL_GROUP_G7;
                // NOTE: Not required since cutter radius compensation is always disabled. Only here
                // to support G40 commands that often appear in g-code program headers to setup defaults.
                // gc_block.modal.cutter_comp = CUTTER_COMP_DISABLE; // G40
                break;

            case 43:
            case 49:
                word_bit = MODAL_GROUP_G8;
                // NOTE: The NIST g-code standard vaguely states that when a tool length offset is changed,
                // there cannot be any axis motion or coordinate offsets updated. Meaning G43, G43.1, and G49
                // all are explicit axis commands, regardless if they require axis words or not.
                if(axis_command)
                {
                    // [Axis word/command conflict]
                    return STATUS_GCODE_AXIS_COMMAND_CONFLICT;
                }

                axis_command = AXIS_COMMAND_TOOL_LENGTH_OFFSET;

                if(int_value == 49) // G49
                {
                    gc_block.modal.tool_length = TOOL_LENGTH_OFFSET_CANCEL;
                }
                else if(mantissa == 10) // G43.1
                {
                    gc_block.modal.tool_length = TOOL_LENGTH_OFFSET_ENABLE_DYNAMIC;
                }
                else if(mantissa < 0.001)   // G43
                {
                    update_tooltable = 1;
                    gc_block.modal.tool_length = TOOL_LENGTH_OFFSET_ENABLE;
                }
                else
                {
                    // [Unsupported G43.x command]
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                // Set to zero to indicate valid non-integer G command.
                mantissa = 0;
                break;

            case 54:
            case 55:
            case 56:
            case 57:
            case 58:
            case 59:
                // NOTE: G59.x are not supported. (But their int_values would be 60, 61, and 62.)
                word_bit = MODAL_GROUP_G12;
                // Shift to array indexing.
                gc_block.modal.coord_select = int_value - 54;
                break;

            case 61:
                word_bit = MODAL_GROUP_G13;
                if(mantissa != 0)
                {
                    // [G61.1 not supported]
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
                // gc_block.modal.control = CONTROL_MODE_EXACT_PATH; // G61
                break;

            default:
                // [Unsupported G command]
                return STATUS_GCODE_UNSUPPORTED_COMMAND;
            }

            if(mantissa > 0)
            {
                // [Unsupported or invalid Gxx.x command]
                return STATUS_GCODE_COMMAND_VALUE_NOT_INTEGER;
            }

            // Check for more than one command per modal group violations in the current block
            // NOTE: Variable 'word_bit' is always assigned, if the command is valid.
            if(BIT_IS_TRUE(command_words,BIT(word_bit)))
            {
                return STATUS_GCODE_MODAL_GROUP_VIOLATION;
            }
            command_words |= BIT(word_bit);
            break;

        case 'M':
            // Determine 'M' command and its modal group
            if(mantissa > 0)
            {
                // [No Mxx.x commands]
                return STATUS_GCODE_COMMAND_VALUE_NOT_INTEGER;
            }

            switch(int_value)
            {
            case 0:
            case 1:
            case 2:
            case 30:
                word_bit = MODAL_GROUP_M4;

                switch(int_value)
                {
                case 0:
                    gc_block.modal.program_flow = PROGRAM_FLOW_PAUSED;
                    break; // Program pause

                case 1:
                    break; // Optional stop not supported. Ignore.

                default:
                    // Program end and reset
                    gc_block.modal.program_flow = int_value;
                }
                break;

            case 3:
            case 4:
            case 5:
                word_bit = MODAL_GROUP_M7;
                switch(int_value)
                {
                case 3:
                    gc_block.modal.spindle = SPINDLE_ENABLE_CW;
                    break;

                case 4:
                    gc_block.modal.spindle = SPINDLE_ENABLE_CCW;
                    break;

                case 5:
                    gc_block.modal.spindle = SPINDLE_DISABLE;
                    break;
                }
                break;

            case 6: // Tool change
                change_tool = 1;
                break;

            case 61: // Set current tool
                apply_tool = 1;
                break;

            case 7:
            case 8:
            case 9:
                word_bit = MODAL_GROUP_M8;

                switch(int_value)
                {
                case 7:
                    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_ENABLE_M7))
                    {
                        gc_block.modal.coolant |= COOLANT_MIST_ENABLE;
                    }
                    else
                    {
                        return STATUS_GCODE_UNSUPPORTED_COMMAND;
                    }
                    break;

                case 8:
                    gc_block.modal.coolant |= COOLANT_FLOOD_ENABLE;
                    break;

                case 9:
                    // M9: Disable both
                    gc_block.modal.coolant = COOLANT_DISABLE;
                    break;
                }
                break;

#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
            case 56:
                word_bit = MODAL_GROUP_M9;
                gc_block.modal.override = OVERRIDE_PARKING_MOTION;
                break;
#endif

            // M62 - M65 Digital Output Control
            case 62:
                // Turn on IO in sync
            case 63:
                // Turn off IO in sync
            case 64:
                // Turn on IO immediately
            case 65:
                // Turn off IO immediately
            case 66:
                // Wait on user input
                word_bit = MODAL_GROUP_M5;
                io_cmd = int_value;
                break;

            default:
                // [Unsupported M command]
                return STATUS_GCODE_UNSUPPORTED_COMMAND;
            }

            // Check for more than one command per modal group violations in the current block
            // NOTE: Variable 'word_bit' is always assigned, if the command is valid.
            if(BIT_IS_TRUE(command_words, BIT(word_bit)))
            {
                return STATUS_GCODE_MODAL_GROUP_VIOLATION;
            }
            command_words |= BIT(word_bit);
            break;

            // NOTE: All remaining letters assign values.
        default:
            /* Non-Command Words: This initial parsing phase only checks for repeats of the remaining
            legal g-code words and stores their value. Error-checking is performed later since some
            words (I,J,K,L,P,R) have multiple connotations and/or depend on the issued commands. */
            switch (letter)
            {
            case 'A':
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_ENABLE_MULTI_AXIS))
                {
                    word_bit = WORD_A;
                    gc_block.values.xyz[A_AXIS] = value;
                    axis_words |= (1 << A_AXIS);
                }
                break;

            case 'B':
                if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_ENABLE_MULTI_AXIS))
                {
                    word_bit = WORD_B;
                    gc_block.values.xyz[B_AXIS] = value;
                    axis_words |= (1 << B_AXIS);
                }
                break;

            // case 'C': // Not supported

            case 'D':
                word_bit = WORD_D;
                gc_block.values.d = int_value;
                break;  // Maybe float?

            case 'F':
                word_bit = WORD_F;
                gc_block.values.f = value;
                break;

            case 'H':
                word_bit = WORD_H;
                gc_block.values.h = int_value;
                break;

            case 'E':
                word_bit = WORD_E;
                gc_block.values.e = value;
                break;

            case 'I':
                word_bit = WORD_I;
                gc_block.values.ijk[X_AXIS] = value;
                ijk_words |= (1<<X_AXIS);
                break;

            case 'J':
                word_bit = WORD_J;
                gc_block.values.ijk[Y_AXIS] = value;
                ijk_words |= (1<<Y_AXIS);
                break;

            case 'K':
                word_bit = WORD_K;
                gc_block.values.ijk[Z_AXIS] = value;
                ijk_words |= (1<<Z_AXIS);
                break;

            case 'L':
                word_bit = WORD_L;
                gc_block.values.l = int_value;
                break;

            case 'N':
                word_bit = WORD_N;
                gc_block.values.n = truncf(value);
                break;

            case 'P':
                // NOTE: For certain commands, P value must be an integer.
                word_bit = WORD_P;
                gc_block.values.p = value;
                break;

            case 'Q':
                word_bit = WORD_Q;
                gc_block.values.q = value;
                break;

            case 'R':
                word_bit = WORD_R;
                gc_block.values.r = value;
                break;

            case 'S':
                word_bit = WORD_S;
                gc_block.values.s = value;
                break;

            case 'T':
                word_bit = WORD_T;
                if(value > MAX_TOOL_NUMBER)
                {
                    return STATUS_GCODE_MAX_VALUE_EXCEEDED;
                }
                gc_block.values.t = int_value;
                break;

            case 'X':
                word_bit = WORD_X;
                gc_block.values.xyz[X_AXIS] = value;
                axis_words |= (1<<X_AXIS);
                break;

            case 'Y':
                word_bit = WORD_Y;
                gc_block.values.xyz[Y_AXIS] = value;
                axis_words |= (1<<Y_AXIS);
                break;

            case 'Z':
                word_bit = WORD_Z;
                gc_block.values.xyz[Z_AXIS] = value;
                axis_words |= (1<<Z_AXIS);
                break;

            default:
                return STATUS_GCODE_UNSUPPORTED_COMMAND;
            }

            // NOTE: Variable 'word_bit' is always assigned, if the non-command letter is valid.
            if (BIT_IS_TRUE(value_words, BIT(word_bit)))
            {
                // [Word repeated]
                return STATUS_GCODE_WORD_REPEATED;
            }

            // Check for invalid negative values for words F, N, P, T, and S.
            // NOTE: Negative value check is done here simply for code-efficiency.
            if(BIT(word_bit) & (BIT(WORD_D)|BIT(WORD_F)|BIT(WORD_N)|BIT(WORD_P)|BIT(WORD_T)|BIT(WORD_S)))
            {
                if(value < 0.0)
                {
                    // [Word value cannot be negative]
                    return STATUS_NEGATIVE_VALUE;
                }
            }
            // Flag to indicate parameter assigned.
            value_words |= BIT(word_bit);
        }
    }
    // Parsing complete!


    /* -------------------------------------------------------------------------------------
    STEP 3: Error-check all commands and values passed in this block. This step ensures all of
    the commands are valid for execution and follows the NIST standard as closely as possible.
    If an error is found, all commands and values in this block are dumped and will not update
    the active system g-code modes. If the block is ok, the active system g-code modes will be
    updated based on the commands of this block, and signal for it to be executed.

    Also, we have to pre-convert all of the values passed based on the modes set by the parsed
    block. There are a number of error-checks that require target information that can only be
    accurately calculated if we convert these values in conjunction with the error-checking.
    This relegates the next execution step as only updating the system g-code modes and
    performing the programmed actions in order. The execution step should not require any
    conversion calculations and would only require minimal checks necessary to execute.
    */

    /* NOTE: At this point, the g-code block has been parsed and the block line can be freed.
    NOTE: It's also possible, at some future point, to break up STEP 2, to allow piece-wise
    parsing of the block on a per-word basis, rather than the entire block. This could remove
    the need for maintaining a large string variable for the entire block and free up some memory.
    To do this, this would simply need to retain all of the data in STEP 1, such as the new block
    data struct, the modal group and value bitflag tracking variables, and axis array indices
    compatible variables. This data contains all of the information necessary to error-check the
    new g-code block when the EOL character is received. However, this would break Grbl's startup
    lines in how it currently works and would require some refactoring to make it compatible.
    */

    // [0. Non-specific/common error-checks and miscellaneous setup]:

    // Determine implicit axis command conditions. Axis words have been passed, but no explicit axis
    // command has been sent. If so, set axis command to current motion mode.
    if(axis_words)
    {
        if(!axis_command)
        {
            // Assign implicit motion-mode
            axis_command = AXIS_COMMAND_MOTION_MODE;
        }
    }

    // Check for valid line number N value.
    if(BIT_IS_TRUE(value_words,BIT(WORD_N)))
    {
        // Line number value cannot be less than zero (done) or greater than max line number.
        if(gc_block.values.n > MAX_LINE_NUMBER)
        {
            // [Exceeds max line number]
            return STATUS_GCODE_INVALID_LINE_NUMBER;
        }
    }

    // bit_false(value_words,bit(WORD_N)); // NOTE: Single-meaning value word. Set at end of error-checking.

    // Track for unused words at the end of error-checking.
    // NOTE: Single-meaning value words are removed all at once at the end of error-checking, because
    // they are always used when present. This was done to save a few bytes of flash. For clarity, the
    // single-meaning value words may be removed as they are used. Also, axis words are treated in the
    // same way. If there is an explicit/implicit axis command, XYZ words are always used and are
    // are removed at the end of error-checking.

    // [1. Comments ]: MSG's NOT SUPPORTED. Comment handling performed by protocol.

    // [2. Set feed rate mode ]: G93 F word missing with G1,G2/3 active, implicitly or explicitly. Feed rate
    //   is not defined after switching to G94 from G93.
    // NOTE: For jogging, ignore prior feed rate mode. Enforce G94 and check for required F word.
    if(gc_parser_flags & GC_PARSER_JOG_MOTION)
    {
        if(BIT_IS_FALSE(value_words, BIT(WORD_F)))
        {
            return STATUS_GCODE_UNDEFINED_FEED_RATE;
        }

        if(gc_block.modal.units == UNITS_MODE_INCHES)
        {
            gc_block.values.f *= MM_PER_INCH;
        }
    }
    else
    {
        if(gc_block.modal.feed_rate == FEED_RATE_MODE_INVERSE_TIME)   // = G93
        {
            // NOTE: G38 can also operate in inverse time, but is undefined as an error. Missing F word check added here.
            if(axis_command == AXIS_COMMAND_MOTION_MODE)
            {
                if((gc_block.modal.motion != MOTION_MODE_NONE) && (gc_block.modal.motion != MOTION_MODE_SEEK))
                {
                    if(BIT_IS_FALSE(value_words, BIT(WORD_F)))
                    {
                        // [F word missing]
                        return STATUS_GCODE_UNDEFINED_FEED_RATE;
                    }
                }
            }
            // NOTE: It seems redundant to check for an F word to be passed after switching from G94 to G93. We would
            // accomplish the exact same thing if the feed rate value is always reset to zero and undefined after each
            // inverse time block, since the commands that use this value already perform undefined checks. This would
            // also allow other commands, following this switch, to execute and not error out needlessly. This code is
            // combined with the above feed rate mode and the below set feed rate error-checking.

            // [3. Set feed rate ]: F is negative (done.)
            // - In inverse time mode: Always implicitly zero the feed rate value before and after block completion.
            // NOTE: If in G93 mode or switched into it from G94, just keep F value as initialized zero or passed F word
            // value in the block. If no F word is passed with a motion command that requires a feed rate, this will error
            // out in the motion modes error-checking. However, if no F word is passed with NO motion command that requires
            // a feed rate, we simply move on and the state feed rate value gets updated to zero and remains undefined.
        }
        else   // = G94
        {
            // - In units per mm mode: If F word passed, ensure value is in mm/min, otherwise push last state value.
            if(gc_state.modal.feed_rate == FEED_RATE_MODE_UNITS_PER_MIN)   // Last state is also G94
            {
                if(BIT_IS_TRUE(value_words, BIT(WORD_F)))
                {
                    if(gc_block.modal.units == UNITS_MODE_INCHES)
                    {
                        gc_block.values.f *= MM_PER_INCH;
                    }
                }
                else
                {
                    // Else, switching to G94 from G93, so don't push last state feed rate. Its undefined or the passed F word value.
                    gc_block.values.f = gc_state.feed_rate; // Push last state feed rate
                }
            }
        }
    }
    // bit_false(value_words,bit(WORD_F)); // NOTE: Single-meaning value word. Set at end of error-checking.

    // [4. Set spindle speed ]: S is negative (done.)
    if(BIT_IS_FALSE(value_words, BIT(WORD_S)))
    {
        gc_block.values.s = gc_state.spindle_speed;
    }
    // bit_false(value_words,bit(WORD_S)); // NOTE: Single-meaning value word. Set at end of error-checking.

    // [5. Select tool ]: NOT SUPPORTED. Only tracks value. T is negative (done.) Not an integer. Greater than max tool value.
    if(BIT_IS_TRUE(value_words, BIT(WORD_T)))
    {
        gc_state.tool = gc_block.values.t;
    }
    BIT_FALSE(value_words, BIT(WORD_T)); // NOTE: Single-meaning value word. Set at end of error-checking.

    // [6. Change tool ]:
    if(update_tooltable == 1)
    {
        if(BIT_IS_TRUE(value_words, BIT(WORD_H)))
        {
            if (gc_block.values.h >= TOOLTABLE_MAX_TOOL_NR)
            {
                return STATUS_GCODE_MAX_VALUE_EXCEEDED;
            }
        }
        BIT_FALSE(value_words, BIT(WORD_H));
    }

    // [Input Output Control ]:
    if (BIT_IS_TRUE(command_words, BIT(MODAL_GROUP_M5)))
    {
        if (io_cmd == 66)
        {
            if (BIT_IS_FALSE(value_words, BIT(WORD_P)) && BIT_IS_FALSE(value_words, BIT(WORD_E)))
            {
                // We need either P or E
                return STATUS_GCODE_VALUE_WORD_MISSING;
            }
            else if (BIT_IS_TRUE(value_words, BIT(WORD_P)) && BIT_IS_TRUE(value_words, BIT(WORD_E)))
            {
                // Only one must be true
                return STATUS_GCODE_WORD_REPEATED;
            }

            if (BIT_IS_TRUE(value_words, BIT(WORD_P)))
            {
                if (gc_block.values.p < 0.0)
                {
                    return STATUS_GCODE_INVALID_TARGET;
                }
                BIT_FALSE(value_words, BIT(WORD_P));
            }
            if (BIT_IS_TRUE(value_words, BIT(WORD_E)))
            {
                if (gc_block.values.e < 0.0)
                {
                    return STATUS_GCODE_INVALID_TARGET;
                }
                BIT_FALSE(value_words, BIT(WORD_E));
            }
            if (BIT_IS_TRUE(value_words, BIT(WORD_L)))
            {
                if(gc_block.values.l > 4)
                {
                    return STATUS_GCODE_MAX_VALUE_EXCEEDED;
                }
                BIT_FALSE(value_words, BIT(WORD_L));
            }
            if (BIT_IS_TRUE(value_words, BIT(WORD_Q)))
            {
                if(gc_block.values.q < 0.0)
                {
                    return STATUS_GCODE_INVALID_TARGET;
                }
                BIT_FALSE(value_words, BIT(WORD_Q));
            }
        }
        else
        {
            if (BIT_IS_FALSE(value_words, BIT(WORD_P)))
            {
                return STATUS_GCODE_VALUE_WORD_MISSING;
            }
            BIT_FALSE(value_words, BIT(WORD_P));
        }
    }

    // [7. Spindle control ]:
    if(BIT_IS_TRUE(command_words, BIT(MODAL_GROUP_G14)) && (gc_block.modal.motion == SPINDLE_SURFACE_MODE))
    {
        if(BIT_IS_FALSE(value_words, BIT(WORD_S)))
        {
            // [S word missing]
            return STATUS_GCODE_VALUE_WORD_MISSING;
        }
        BIT_FALSE(value_words, BIT(WORD_S));

        if(BIT_IS_TRUE(value_words, BIT(WORD_D)))
        {
            if(gc_block.values.d == 0)
            {
                return STATUS_INVALID_STATEMENT;
            }
        }
        else
        {
            // Reset spindle limit if not specified (optional)
            gc_block.values.d = 0;
        }
        BIT_FALSE(value_words, BIT(WORD_D));
    }

    // [8. Coolant control ]: N/A
    // [9. Override control ]: Not supported except for a Grbl-only parking motion override control.
#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
    if(BIT_IS_TRUE(command_words, BIT(MODAL_GROUP_M9)))   // Already set as enabled in parser.
    {
        if(BIT_IS_TRUE(value_words, BIT(WORD_P)))
        {
            if(gc_block.values.p == 0.0)
            {
                gc_block.modal.override = OVERRIDE_DISABLED;
            }
            BIT_FALSE(value_words, BIT(WORD_P));
        }
    }
#endif

    // [10. Dwell ]: P value missing. P is negative (done.) NOTE: See below.
    if(gc_block.non_modal_command == NON_MODAL_DWELL)
    {
        if(BIT_IS_FALSE(value_words, BIT(WORD_P)))
        {
            // [P word missing]
            return STATUS_GCODE_VALUE_WORD_MISSING;
        }
        BIT_FALSE(value_words, BIT(WORD_P));
    }

    // [10.1 Canned drilling cycle]: R/P/Q value missing.
    if (gc_block.modal.motion == MOTION_MODE_DRILL || gc_block.modal.motion == MOTION_MODE_DRILL_DWELL || gc_block.modal.motion == MOTION_MODE_DRILL_PECK || gc_block.modal.motion == MOTION_MODE_DRILL_BREAK)
    {
        if(BIT_IS_FALSE(value_words, BIT(WORD_R)))
        {
            // [R word missing]
            return STATUS_GCODE_VALUE_WORD_MISSING;
        }
        BIT_FALSE(value_words, BIT(WORD_R));

        if(gc_block.modal.motion == MOTION_MODE_DRILL_DWELL)
        {
            if(BIT_IS_FALSE(value_words, BIT(WORD_P)))
            {
                // [P word missing]
                return STATUS_GCODE_VALUE_WORD_MISSING;
            }
            // TODO: Check validity of P
            BIT_FALSE(value_words, BIT(WORD_P));
        }
        if (gc_block.modal.motion == MOTION_MODE_DRILL_PECK || gc_block.modal.motion == MOTION_MODE_DRILL_BREAK)
        {
            if(BIT_IS_FALSE(value_words, BIT(WORD_Q)))
            {
                // [Q word missing]
                return STATUS_GCODE_VALUE_WORD_MISSING;
            }
            // TODO: Check validity of Q
            BIT_FALSE(value_words, BIT(WORD_Q));
        }

        // TODO: Inverse time feed rate not allowed for canned cycles
        //if(gc_block.modal.feed_rate == FEED_RATE_MODE_INVERSE_TIME)
    }

    // [11. Set active plane ]: N/A
    switch(gc_block.modal.plane_select)
    {
    case PLANE_SELECT_XY:
        axis_0 = X_AXIS;
        axis_1 = Y_AXIS;
        axis_linear = Z_AXIS;
        break;

    case PLANE_SELECT_ZX:
        axis_0 = Z_AXIS;
        axis_1 = X_AXIS;
        axis_linear = Y_AXIS;
        break;

    default: // case PLANE_SELECT_YZ:
        axis_0 = Y_AXIS;
        axis_1 = Z_AXIS;
        axis_linear = X_AXIS;
    }

    // [12. Set length units ]: N/A
    // Pre-convert XYZ coordinate values to millimeters, if applicable.
    uint8_t idx;
    if(gc_block.modal.units == UNITS_MODE_INCHES)
    {
        // Axes indices are consistent, so loop may be used.
        for(idx = 0; idx < N_AXIS; idx++)
        {
            if(BIT_IS_TRUE(axis_words, BIT(idx)))
            {
                gc_block.values.xyz[idx] *= MM_PER_INCH;
            }
        }
    }

    // [13. Cutter radius compensation ]: G41/42 NOT SUPPORTED. Error, if enabled while G53 is active.
    // [G40 Errors]: G2/3 arc is programmed after a G40. The linear move after disabling is less than tool diameter.
    //   NOTE: Since cutter radius compensation is never enabled, these G40 errors don't apply. Grbl supports G40
    //   only for the purpose to not error when G40 is sent with a g-code program header to setup the default modes.

    // [14. Cutter length compensation ]:
    // [G43.1 Errors]: Motion command in same line.
    //   NOTE: Although not explicitly stated so, G43.1 should be applied to only one valid
    //   axis that is configured (in config.h). There should be an error if the configured axis
    //   is absent or if any of the other axis words are present.
    if((axis_command == AXIS_COMMAND_TOOL_LENGTH_OFFSET) && (update_tooltable == 0))  // Indicates called in block.
    {
        if(gc_block.modal.tool_length == TOOL_LENGTH_OFFSET_ENABLE_DYNAMIC)
        {
            if(axis_words ^ (1<<TOOL_LENGTH_OFFSET_AXIS))
            {
                return STATUS_GCODE_G43_DYNAMIC_AXIS_ERROR;
            }
        }
        if (gc_block.modal.tool_length == TOOL_LENGTH_OFFSET_ENABLE)
        {
            if (BIT_IS_TRUE(value_words, BIT(WORD_H)))
            {
                if (gc_block.values.h > TOOLTABLE_MAX_TOOL_NR - 1)
                {
                    return STATUS_GCODE_MAX_VALUE_EXCEEDED;
                }
            }
            BIT_FALSE(value_words, BIT(WORD_H));
        }
    }

    // [15. Coordinate system selection ]: *N/A. Error, if cutter radius comp is active.
    // TODO: An EEPROM read of the coordinate data may require a buffer sync when the cycle
    // is active. The read pauses the processor temporarily and may cause a rare crash. For
    // future versions on processors with enough memory, all coordinate data should be stored
    // in memory and written to EEPROM only when there is not a cycle active.
    float block_coord_system[N_AXIS];
    memcpy(block_coord_system, gc_state.coord_system, sizeof(gc_state.coord_system));

    // Check if called in block
    if(BIT_IS_TRUE(command_words,BIT(MODAL_GROUP_G12)))
    {
        if(gc_block.modal.coord_select > N_COORDINATE_SYSTEM)
        {
            // [Greater than N sys]
            return STATUS_GCODE_UNSUPPORTED_COORD_SYS;
        }

        if(gc_state.modal.coord_select != gc_block.modal.coord_select)
        {
            if(!(Settings_ReadCoordData(gc_block.modal.coord_select, block_coord_system)))
            {
                return STATUS_SETTING_READ_FAIL;
            }
        }
    }

    // [16. Set path control mode ]: N/A. Only G61. G61.1 and G64 NOT SUPPORTED.
    // [17. Set distance mode ]: N/A. Only G91.1. G90.1 NOT SUPPORTED.
    // [18. Set retract mode ]:

    // [19. Remaining non-modal actions ]: Check go to predefined position, set G10, or set axis offsets.
    // NOTE: We need to separate the non-modal commands that are axis word-using (G10/G28/G30/G92), as these
    // commands all treat axis words differently. G10 as absolute offsets or computes current position as
    // the axis value, G92 similarly to G10 L20, and G28/30 as an intermediate target position that observes
    // all the current coordinate system and G92 offsets.
    switch (gc_block.non_modal_command)
    {
    case NON_MODAL_SET_COORDINATE_DATA:
        // [G10 Errors]: L missing and is not 2 or 20. P word missing. (Negative P value done.)
        // [G10 L2 Errors]: R word NOT SUPPORTED. P value not 0 to nCoordSys(max 9). Axis words missing.
        // [G10 L20 Errors]: P must be 0 to nCoordSys(max 9). Axis words missing.
        if(!axis_words)
        {
            // [No axis words]
            return STATUS_GCODE_NO_AXIS_WORDS;
        }
        if(BIT_IS_FALSE(value_words, ((1<<WORD_P)|(1<<WORD_L))))
        {
            // [P/L word missing]
            return STATUS_GCODE_VALUE_WORD_MISSING;
        }

        // Convert p value to int.
        coord_select = truncf(gc_block.values.p);
        if(coord_select > N_COORDINATE_SYSTEM)
        {
            // [Greater than N sys]
            return STATUS_GCODE_UNSUPPORTED_COORD_SYS;
        }

        if(gc_block.values.l != 20)
        {
            if(gc_block.values.l == 2)
            {
                if(BIT_IS_TRUE(value_words, BIT(WORD_R)))
                {
                    // [G10 L2 R not supported]
                    return STATUS_GCODE_UNSUPPORTED_COMMAND;
                }
            }
            else
            {
                // [Unsupported L]
                return STATUS_GCODE_UNSUPPORTED_COMMAND;
            }
        }
        BIT_FALSE(value_words, (BIT(WORD_L) | BIT(WORD_P)));

        // Determine coordinate system to change and try to load from EEPROM.
        if(coord_select > 0)
        {
            // Adjust P1-P6 index to EEPROM coordinate data indexing.
            coord_select--;
        }
        else
        {
            // Index P0 as the active coordinate system
            coord_select = gc_block.modal.coord_select;
        }

        // NOTE: Store parameter data in IJK values. By rule, they are not in use with this command.
        if(!Settings_ReadCoordData(coord_select, gc_block.values.ijk))
        {
            // [EEPROM read fail]
            return STATUS_SETTING_READ_FAIL;
        }

        // Pre-calculate the coordinate data changes.
        for(idx = 0; idx < N_AXIS; idx++)   // Axes indices are consistent, so loop may be used.
        {
            // Update axes defined only in block. Always in machine coordinates. Can change non-active system.
            if(BIT_IS_TRUE(axis_words, BIT(idx)))
            {
                if(gc_block.values.l == 20)
                {
                    // L20: Update coordinate system axis at current position (with modifiers) with programmed value
                    // WPos = MPos - WCS - G92 - TLO  ->  WCS = MPos - G92 - TLO - WPos
                    gc_block.values.ijk[idx] = gc_state.position[idx] - gc_state.coord_offset[idx] - gc_block.values.xyz[idx];
                    //if(idx == TOOL_LENGTH_OFFSET_AXIS)
                    {
                        gc_block.values.ijk[idx] -= gc_state.tool_length_offset_dynamic[idx] + gc_state.tool_length_offset[idx];
                    }
                }
                else
                {
                    // L2: Update coordinate system axis to programmed value.
                    gc_block.values.ijk[idx] = gc_block.values.xyz[idx];
                }
            } // Else, keep current stored value.
        }
        break;

    case NON_MODAL_SET_COORDINATE_OFFSET:
        // [G92 Errors]: No axis words.
        if(!axis_words)
        {
            // [No axis words]
            return STATUS_GCODE_NO_AXIS_WORDS;
        }

        // Update axes defined only in block. Offsets current system to defined value. Does not update when
        // active coordinate system is selected, but is still active unless G92.1 disables it.
        for(idx = 0; idx < N_AXIS; idx++)   // Axes indices are consistent, so loop may be used.
        {
            if(BIT_IS_TRUE(axis_words, BIT(idx)))
            {
                // WPos = MPos - WCS - G92 - TLO  ->  G92 = MPos - WCS - TLO - WPos
                gc_block.values.xyz[idx] = gc_state.position[idx] - block_coord_system[idx] - gc_block.values.xyz[idx];
                //if(idx == TOOL_LENGTH_OFFSET_AXIS)
                {
                    gc_block.values.xyz[idx] -= gc_state.tool_length_offset_dynamic[idx] + gc_state.tool_length_offset[idx];
                }
            }
            else
            {
                gc_block.values.xyz[idx] = gc_state.coord_offset[idx];
            }
        }
        break;

    default:
        // At this point, the rest of the explicit axis commands treat the axis values as the traditional
        // target position with the coordinate system offsets, G92 offsets, absolute override, and distance
        // modes applied. This includes the motion mode commands. We can now pre-compute the target position.
        // NOTE: Tool offsets may be appended to these conversions when/if this feature is added.
        if(axis_command != AXIS_COMMAND_TOOL_LENGTH_OFFSET)   // TLO block any axis command.
        {
            if(axis_words)
            {
                for(idx = 0; idx < N_AXIS; idx++)
                {
                    if(BIT_IS_FALSE(axis_words, BIT(idx)))
                    {
                        // No axis word in block. Keep same axis position.
                        gc_block.values.xyz[idx] = gc_state.position[idx];
                    }
                    else
                    {
                        // Update specified value according to distance mode or ignore if absolute override is active.
                        // NOTE: G53 is never active with G28/30 since they are in the same modal group.
                        if(gc_block.non_modal_command != NON_MODAL_ABSOLUTE_OVERRIDE)
                        {
                            // Apply coordinate offsets based on distance mode.
                            if(gc_block.modal.distance == DISTANCE_MODE_ABSOLUTE)
                            {
                                gc_block.values.xyz[idx] += block_coord_system[idx] + gc_state.coord_offset[idx];
                                //if(idx == TOOL_LENGTH_OFFSET_AXIS)
                                {
                                    gc_block.values.xyz[idx] += gc_state.tool_length_offset_dynamic[idx] + gc_state.tool_length_offset[idx];
                                }
                            }
                            else
                            {
                                // Incremental mode
                                gc_block.values.xyz[idx] += gc_state.position[idx];
                            }
                        }
                    }
                }
            }
        }

        // Check remaining non-modal commands for errors.
        switch(gc_block.non_modal_command)
        {
        case NON_MODAL_GO_HOME_0: // G28
        case NON_MODAL_GO_HOME_1: // G30
            // [G28/30 Errors]: Cutter compensation is enabled.
            // Retreive G28/30 go-home position data (in machine coordinates) from EEPROM
            // NOTE: Store parameter data in IJK values. By rule, they are not in use with this command.
            if(gc_block.non_modal_command == NON_MODAL_GO_HOME_0)
            {
                if(!Settings_ReadCoordData(SETTING_INDEX_G28, gc_block.values.ijk))
                {
                    return STATUS_SETTING_READ_FAIL;
                }
            }
            else // == NON_MODAL_GO_HOME_1
            {
                if(!Settings_ReadCoordData(SETTING_INDEX_G30, gc_block.values.ijk))
                {
                    return STATUS_SETTING_READ_FAIL;
                }
            }

            if(axis_words)
            {
                // Move only the axes specified in secondary move.
                for(idx = 0; idx < N_AXIS; idx++)
                {
                    if(!(axis_words & (1<<idx)))
                    {
                        gc_block.values.ijk[idx] = gc_state.position[idx];
                    }
                }
            }
            else
            {
                // Set to none if no intermediate motion.
                axis_command = AXIS_COMMAND_NONE;
            }
            break;

        case NON_MODAL_SET_HOME_0: // G28.1
        case NON_MODAL_SET_HOME_1: // G30.1
            // [G28.1/30.1 Errors]: Cutter compensation is enabled.
            // NOTE: If axis words are passed here, they are interpreted as an implicit motion mode.
            break;

        case NON_MODAL_RESET_COORDINATE_OFFSET:
            // NOTE: If axis words are passed here, they are interpreted as an implicit motion mode.
            break;

        case NON_MODAL_ABSOLUTE_OVERRIDE:
            // [G53 Errors]: G0 and G1 are not active. Cutter compensation is enabled.
            // NOTE: All explicit axis word commands are in this modal group. So no implicit check necessary.
            if(!(gc_block.modal.motion == MOTION_MODE_SEEK || gc_block.modal.motion == MOTION_MODE_LINEAR))
            {
                // [G53 G0/1 not active]
                return STATUS_GCODE_G53_INVALID_MOTION_MODE;
            }
            break;
        }
    }

    // [20. Motion modes ]:
    if(gc_block.modal.motion == MOTION_MODE_NONE)
    {
        // [G80 Errors]: Axis word are programmed while G80 is active.
        // NOTE: Even non-modal commands or TLO that use axis words will throw this strict error.
        if(axis_words)
        {
            // [No axis words allowed]
            return STATUS_GCODE_AXIS_WORDS_EXIST;
        }

        // Check remaining motion modes, if axis word are implicit (exist and not used by G10/28/30/92), or
        // was explicitly commanded in the g-code block.
    }
    else if(axis_command == AXIS_COMMAND_MOTION_MODE)
    {
        if(gc_block.modal.motion == MOTION_MODE_SEEK)
        {
            // [G0 Errors]: Axis letter not configured or without real value (done.)
            // Axis words are optional. If missing, set axis command flag to ignore execution.
            if(!axis_words)
            {
                axis_command = AXIS_COMMAND_NONE;
            }

            // All remaining motion modes (all but G0, G80, G33 and G76), require a valid feed rate value. In units per mm mode,
            // the value must be positive. In inverse time mode, a positive value must be passed with each block.
        }
        else if((gc_block.modal.motion == MOTION_MODE_SPINDLE_SYNC) || (gc_block.modal.motion == MOTION_MODE_THREADING))
        {
            switch(gc_block.modal.motion)
            {
            case MOTION_MODE_SPINDLE_SYNC:
                if(BIT_IS_FALSE(value_words, BIT(WORD_K)))
                {
                    // [K word missing]
                    return STATUS_GCODE_VALUE_WORD_MISSING;
                }
                BIT_FALSE(value_words, BIT(WORD_K));

                if(BIT_IS_FALSE(value_words, (BIT(WORD_X) | BIT(WORD_Y) | BIT(WORD_Z))))
                {
                    // [axis word missing]
                    return STATUS_GCODE_NO_AXIS_WORDS;
                }

                break;

            case MOTION_MODE_THREADING:
                if(BIT_IS_FALSE(value_words, BIT(WORD_P)))
                {
                    // [P word missing]
                    return STATUS_GCODE_VALUE_WORD_MISSING;
                }
                BIT_FALSE(value_words, BIT(WORD_P));

                if(BIT_IS_FALSE(value_words, BIT(WORD_Z)))
                {
                    // [axis word missing]
                    return STATUS_GCODE_NO_AXIS_WORDS;
                }

                if(BIT_IS_FALSE(value_words, (BIT(WORD_I) | BIT(WORD_J) | BIT(WORD_K))))
                {
                    // [IJK word missing]
                    return STATUS_GCODE_VALUE_WORD_MISSING;
                }
                BIT_FALSE(value_words, (BIT(WORD_I) | BIT(WORD_J) | BIT(WORD_K)));

                // [Optional]
                if(BIT_IS_TRUE(value_words, BIT(WORD_R)))
                {
                    if(gc_block.values.r < 1.0)
                    {
                        return STATUS_BAD_NUMBER_FORMAT;
                    }
                }

                if(BIT_IS_TRUE(value_words, BIT(WORD_L)))
                {
                    if(gc_block.values.l > 3)
                    {
                        return STATUS_BAD_NUMBER_FORMAT;
                    }
                }

                if(BIT_IS_TRUE(value_words, BIT(WORD_Q)))
                {
                    if(gc_block.values.q < 0.0 || gc_block.values.q > 80)
                    {
                        return STATUS_BAD_NUMBER_FORMAT;
                    }
                }

                BIT_FALSE(value_words, (BIT(WORD_R) | BIT(WORD_Q) | BIT(WORD_H) | BIT(WORD_E) | BIT(WORD_L)));
                break;
            }
        }
        else
        {
            // Check if feed rate is defined for the motion modes that require it.
            if(gc_block.values.f == 0.0)
            {
                // [Feed rate undefined]
                return STATUS_GCODE_UNDEFINED_FEED_RATE;
            }

            switch(gc_block.modal.motion)
            {
            case MOTION_MODE_LINEAR:
                // [G1 Errors]: Feed rate undefined. Axis letter not configured or without real value.
                // Axis words are optional. If missing, set axis command flag to ignore execution.
                if(!axis_words)
                {
                    axis_command = AXIS_COMMAND_NONE;
                }
                break;

            case MOTION_MODE_CW_ARC:
                // No break intentional.
                gc_parser_flags |= GC_PARSER_ARC_IS_CLOCKWISE;

            case MOTION_MODE_CCW_ARC:
                // [G2/3 Errors All-Modes]: Feed rate undefined.
                // [G2/3 Radius-Mode Errors]: No axis words in selected plane. Target point is same as current.
                // [G2/3 Offset-Mode Errors]: No axis words and/or offsets in selected plane. The radius to the current
                //   point and the radius to the target point differs more than 0.002mm (EMC def. 0.5mm OR 0.005mm and 0.1% radius).
                // [G2/3 Full-Circle-Mode Errors]: NOT SUPPORTED. Axis words exist. No offsets programmed. P must be an integer.
                // NOTE: Both radius and offsets are required for arc tracing and are pre-computed with the error-checking.

                if(!axis_words)
                {
                    // [No axis words]
                    return STATUS_GCODE_NO_AXIS_WORDS;
                }

                if(!(axis_words & (BIT(axis_0)|BIT(axis_1))))
                {
                    // [No axis words in plane]
                    return STATUS_GCODE_NO_AXIS_WORDS_IN_PLANE;
                }

                // Calculate the change in position along each selected axis
                float x, y;
                x = gc_block.values.xyz[axis_0]-gc_state.position[axis_0]; // Delta x between current position and target
                y = gc_block.values.xyz[axis_1]-gc_state.position[axis_1]; // Delta y between current position and target

                // Arc Radius Mode
                if(value_words & BIT(WORD_R))
                {
                    BIT_FALSE(value_words, BIT(WORD_R));
                    if(isequal_position_vector(gc_state.position, gc_block.values.xyz))
                    {
                        // [Invalid target]
                        return STATUS_GCODE_INVALID_TARGET;
                    }

                    // Convert radius value to proper units.
                    if(gc_block.modal.units == UNITS_MODE_INCHES)
                    {
                        gc_block.values.r *= MM_PER_INCH;
                    }
                    /*  We need to calculate the center of the circle that has the designated radius and passes
                    through both the current position and the target position. This method calculates the following
                    set of equations where [x,y] is the vector from current to target position, d == magnitude of
                    that vector, h == hypotenuse of the triangle formed by the radius of the circle, the distance to
                    the center of the travel vector. A vector perpendicular to the travel vector [-y,x] is scaled to the
                    length of h [-y/d*h, x/d*h] and added to the center of the travel vector [x/2,y/2] to form the new point
                    [i,j] at [x/2-y/d*h, y/2+x/d*h] which will be the center of our arc.

                    d^2 == x^2 + y^2
                    h^2 == r^2 - (d/2)^2
                    i == x/2 - y/d*h
                    j == y/2 + x/d*h

                                                                        O <- [i,j]
                                                                        -  |
                                                            r      -     |
                                                                -        |
                                                            -           | h
                                                            -              |
                                            [0,0] ->  C -----------------+--------------- T  <- [x,y]
                                                        | <------ d/2 ---->|

                    C - Current position
                    T - Target position
                    O - center of circle that pass through both C and T
                    d - distance from C to T
                    r - designated radius
                    h - distance from center of CT to O

                    Expanding the equations:

                    d -> sqrtf(x^2 + y^2)
                    h -> sqrtf(4 * r^2 - x^2 - y^2)/2
                    i -> (x - (y * sqrtf(4 * r^2 - x^2 - y^2)) / sqrtf(x^2 + y^2)) / 2
                    j -> (y + (x * sqrtf(4 * r^2 - x^2 - y^2)) / sqrtf(x^2 + y^2)) / 2

                    Which can be written:

                    i -> (x - (y * sqrtf(4 * r^2 - x^2 - y^2))/sqrtf(x^2 + y^2))/2
                    j -> (y + (x * sqrtf(4 * r^2 - x^2 - y^2))/sqrtf(x^2 + y^2))/2

                    Which we for size and speed reasons optimize to:

                    h_x2_div_d = sqrtf(4 * r^2 - x^2 - y^2)/sqrtf(x^2 + y^2)
                    i = (x - (y * h_x2_div_d))/2
                    j = (y + (x * h_x2_div_d))/2
                    */

                    // First, use h_x2_div_d to compute 4*h^2 to check if it is negative or r is smaller
                    // than d. If so, the sqrt of a negative number is complex and error out.
                    float h_x2_div_d = 4.0 * gc_block.values.r*gc_block.values.r - x*x - y*y;

                    if(h_x2_div_d < 0)
                    {
                        // [Arc radius error]
                        return STATUS_GCODE_ARC_RADIUS_ERROR;
                    }

                    // Finish computing h_x2_div_d.
                    //h_x2_div_d = -sqrtf(h_x2_div_d) / hypot_f(x,y); // == -(h * 2 / d)
                    h_x2_div_d = -sqrtf(h_x2_div_d) / hypotf(x, y); // == -(h * 2 / d)
                    // Invert the sign of h_x2_div_d if the circle is counter clockwise (see sketch below)
                    if(gc_block.modal.motion == MOTION_MODE_CCW_ARC)
                    {
                        h_x2_div_d = -h_x2_div_d;
                    }

                    /* The counter clockwise circle lies to the left of the target direction. When offset is positive,
                    the left hand circle will be generated - when it is negative the right hand circle is generated.

                                                                            T  <-- Target position

                                                                            ^
                                Clockwise circles with this center         |          Clockwise circles with this center will have
                                will have > 180 deg of angular travel      |          < 180 deg of angular travel, which is a good thing!
                                                                \         |          /
                    center of arc when h_x2_div_d is positive ->  x <----- | -----> x <- center of arc when h_x2_div_d is negative
                                                                            |
                                                                            |

                                                                            C  <-- Current position
                    */
                    // Negative R is g-code-alese for "I want a circle with more than 180 degrees of travel" (go figure!),
                    // even though it is advised against ever generating such circles in a single line of g-code. By
                    // inverting the sign of h_x2_div_d the center of the circles is placed on the opposite side of the line of
                    // travel and thus we get the unadvisably long arcs as prescribed.
                    if(gc_block.values.r < 0.0)
                    {
                        h_x2_div_d = -h_x2_div_d;
                        gc_block.values.r = -gc_block.values.r; // Finished with r. Set to positive for mc_arc
                    }

                    // Complete the operation by calculating the actual center of the arc
                    gc_block.values.ijk[axis_0] = 0.5*(x-(y*h_x2_div_d));
                    gc_block.values.ijk[axis_1] = 0.5*(y+(x*h_x2_div_d));

                }
                else   // Arc Center Format Offset Mode
                {
                    if(!(ijk_words & (BIT(axis_0)|BIT(axis_1))))
                    {
                        // [No offsets in plane]
                        return STATUS_GCODE_NO_OFFSETS_IN_PLANE;
                    }
                    BIT_FALSE(value_words, (BIT(WORD_I) | BIT(WORD_J) | BIT(WORD_K)));

                    // Convert IJK values to proper units.
                    if(gc_block.modal.units == UNITS_MODE_INCHES)
                    {
                        for(idx = 0; idx < N_LINEAR_AXIS; idx++)
                        {
                            if(ijk_words & BIT(idx))
                            {
                                gc_block.values.ijk[idx] *= MM_PER_INCH;
                            }
                        }
                    }

                    // Arc radius from center to target
                    x -= gc_block.values.ijk[axis_0]; // Delta x between circle center and target
                    y -= gc_block.values.ijk[axis_1]; // Delta y between circle center and target

                    //float target_r = hypot_f(x,y);
                    float target_r = hypotf(x, y);

                    // Compute arc radius for mc_arc. Defined from current location to center.
                    //gc_block.values.r = hypot_f(gc_block.values.ijk[axis_0], gc_block.values.ijk[axis_1]);
                    gc_block.values.r = hypotf(gc_block.values.ijk[axis_0], gc_block.values.ijk[axis_1]);

                    // Compute difference between current location and target radii for final error-checks.
                    float delta_r = fabsf(target_r-gc_block.values.r);
                    if(delta_r > 0.005)
                    {
                        if(delta_r > 0.5)
                        {
                            // [Arc definition error] > 0.5mm
                            return STATUS_GCODE_INVALID_TARGET;
                        }
                        if(delta_r > (0.001*gc_block.values.r))
                        {
                            // [Arc definition error] > 0.005mm AND 0.1% radius
                            return STATUS_GCODE_INVALID_TARGET;
                        }
                    }
                }
                break;

            case MOTION_MODE_PROBE_TOWARD_NO_ERROR:
            case MOTION_MODE_PROBE_AWAY_NO_ERROR:
                gc_parser_flags |= GC_PARSER_PROBE_IS_NO_ERROR; // No break intentional.

            case MOTION_MODE_PROBE_TOWARD:
            case MOTION_MODE_PROBE_AWAY:
                if((gc_block.modal.motion == MOTION_MODE_PROBE_AWAY) || (gc_block.modal.motion == MOTION_MODE_PROBE_AWAY_NO_ERROR))
                {
                    gc_parser_flags |= GC_PARSER_PROBE_IS_AWAY;
                }
                // [G38 Errors]: Target is same current. No axis words. Cutter compensation is enabled. Feed rate
                //   is undefined. Probe is triggered. NOTE: Probe check moved to probe cycle. Instead of returning
                //   an error, it issues an alarm to prevent further motion to the probe. It's also done there to
                //   allow the planner buffer to empty and move off the probe trigger before another probing cycle.
                if(!axis_words)
                {
                    // [No axis words]
                    return STATUS_GCODE_NO_AXIS_WORDS;
                }

                if(isequal_position_vector(gc_state.position, gc_block.values.xyz))
                {
                    // [Invalid target]
                    return STATUS_GCODE_INVALID_TARGET;
                }
                break;

            case MOTION_MODE_DRILL:
            case MOTION_MODE_DRILL_DWELL:
            case MOTION_MODE_DRILL_PECK:
            case MOTION_MODE_DRILL_BREAK:
                if(BIT_TRUE(value_words, (BIT(WORD_L))))
                {
                    // Optional
                }
                BIT_FALSE(value_words, BIT(WORD_L));
                break;
            }
        }
    }

    // [21. Program flow ]: No error checks required.

    // [0. Non-specific error-checks]: Complete unused value words check, i.e. IJK used when in arc
    // radius mode, or axis words that aren't used in the block.
    if(gc_parser_flags & GC_PARSER_JOG_MOTION)
    {
        // Jogging only uses the F feed rate and XYZ value words. N is valid, but S and T are invalid.
        BIT_FALSE(value_words, (BIT(WORD_N)|BIT(WORD_F)));
    }
    else
    {
        BIT_FALSE(value_words, (BIT(WORD_N)|BIT(WORD_F)|BIT(WORD_S)|BIT(WORD_T))); // Remove single-meaning value words.
    }

    if(axis_command)
    {
        // Remove axis words.
        BIT_FALSE(value_words, (BIT(WORD_X)|BIT(WORD_Y)|BIT(WORD_Z)|BIT(WORD_A)|BIT(WORD_B)));
    }

    if(value_words)
    {
        // [Unused words]
        return STATUS_GCODE_UNUSED_WORDS;
    }

    /* -------------------------------------------------------------------------------------
    STEP 4: EXECUTE!!
    Assumes that all error-checking has been completed and no failure modes exist. We just
    need to update the state and execute the block according to the order-of-execution.
    */

    // Initialize planner data struct for motion blocks.
    Planner_LineData_t plan_data;
    Planner_LineData_t *pl_data = &plan_data;
    memset(pl_data, 0, sizeof(Planner_LineData_t)); // Zero pl_data struct

    if ((BIT_IS_TRUE(settings.flags_ext, BITFLAG_LATHE_MODE)) && gc_block.modal.lathe_mode == LATHE_DIAMETER_MODE)
    {
        gc_block.values.xyz[X_AXIS] /= 2.0;
    }

    // Intercept jog commands and complete error checking for valid jog commands and execute.
    // NOTE: G-code parser state is not updated, except the position to ensure sequential jog
    // targets are computed correctly. The final parser position after a jog is updated in
    // protocol_execute_realtime() when jogging completes or is canceled.
    if(gc_parser_flags & GC_PARSER_JOG_MOTION)
    {
        // Only distance and unit modal commands and G53 absolute override command are allowed.
        // NOTE: Feed rate word and axis word checks have already been performed in STEP 3.
        if(command_words & ~(BIT(MODAL_GROUP_G3) | BIT(MODAL_GROUP_G6) | BIT(MODAL_GROUP_G0)))
        {
            return STATUS_INVALID_JOG_COMMAND;
        }

        if(!(gc_block.non_modal_command == NON_MODAL_ABSOLUTE_OVERRIDE || gc_block.non_modal_command == NON_MODAL_NO_ACTION))
        {
            return STATUS_INVALID_JOG_COMMAND;
        }

        // Initialize planner data to current spindle and coolant modal state.
        pl_data->spindle_speed = gc_state.spindle_speed;
        plan_data.condition = (gc_state.modal.spindle | gc_state.modal.coolant);

        uint8_t status = Jog_Execute(&plan_data, &gc_block);
        if(status == STATUS_OK)
        {
            memcpy(gc_state.position, gc_block.values.xyz, sizeof(gc_block.values.xyz));
        }

        return status;
    }

    // If in laser mode, setup laser power based on current and past parser conditions.
    if(BIT_IS_TRUE(settings.flags, BITFLAG_LASER_MODE))
    {
        if(!((gc_block.modal.motion == MOTION_MODE_LINEAR) || (gc_block.modal.motion == MOTION_MODE_CW_ARC) || (gc_block.modal.motion == MOTION_MODE_CCW_ARC)))
        {
            gc_parser_flags |= GC_PARSER_LASER_DISABLE;
        }

        // Any motion mode with axis words is allowed to be passed from a spindle speed update.
        // NOTE: G1 and G0 without axis words sets axis_command to none. G28/30 are intentionally omitted.
        // TODO: Check sync conditions for M3 enabled motions that don't enter the planner. (zero length).
        if(axis_words && (axis_command == AXIS_COMMAND_MOTION_MODE))
        {
            gc_parser_flags |= GC_PARSER_LASER_ISMOTION;
        }
        else
        {
            // M3 constant power laser requires planner syncs to update the laser when changing between
            // a G1/2/3 motion mode state and vice versa when there is no motion in the line.
            if(gc_state.modal.spindle == SPINDLE_ENABLE_CW)
            {
                if((gc_state.modal.motion == MOTION_MODE_LINEAR) || (gc_state.modal.motion == MOTION_MODE_CW_ARC) || (gc_state.modal.motion == MOTION_MODE_CCW_ARC))
                {
                    if (BIT_IS_TRUE(gc_parser_flags, GC_PARSER_LASER_DISABLE))
                    {
                        // Change from G1/2/3 motion mode.
                        gc_parser_flags |= GC_PARSER_LASER_FORCE_SYNC;
                    }
                }
                else
                {
                    // When changing to a G1 motion mode without axis words from a non-G1/2/3 motion mode.
                    if(BIT_IS_FALSE(gc_parser_flags, GC_PARSER_LASER_DISABLE))
                    {
                        gc_parser_flags |= GC_PARSER_LASER_FORCE_SYNC;
                    }
                }
            }
        }
    }

    // [0. Non-specific/common error-checks and miscellaneous setup]:
    // NOTE: If no line number is present, the value is zero.
    gc_state.line_number = gc_block.values.n;
    // Record data for planner use.
    pl_data->line_number = gc_state.line_number;

    // [1. Comments feedback ]:  NOT SUPPORTED

    // [2. Set feed rate mode ]:
    gc_state.modal.feed_rate = gc_block.modal.feed_rate;
    if(gc_state.modal.feed_rate)
    {
        // Set condition flag for planner use.
        pl_data->condition |= PL_COND_FLAG_INVERSE_TIME;
    }

    // [3. Set feed rate ]:
    gc_state.feed_rate = gc_block.values.f; // Always copy this value. See feed rate error-checking.
    pl_data->feed_rate = gc_state.feed_rate; // Record data for planner use.

    // [4. Set spindle speed ]:
    if((gc_block.modal.spindle_mode == SPINDLE_RPM_MODE) && ((gc_state.spindle_speed != gc_block.values.s) || BIT_IS_TRUE(gc_parser_flags,GC_PARSER_LASER_FORCE_SYNC)))
    {
        if(gc_state.modal.spindle != SPINDLE_DISABLE)
        {
            if(BIT_IS_FALSE(gc_parser_flags, GC_PARSER_LASER_ISMOTION))
            {
                if(BIT_IS_TRUE(gc_parser_flags, GC_PARSER_LASER_DISABLE))
                {
                    Spindle_Sync(gc_state.modal.spindle, 0.0);
                }
                else
                {
                    Spindle_Sync(gc_state.modal.spindle, gc_block.values.s);
                }
            }
        }
        // Update spindle speed state.
        gc_state.spindle_speed = gc_block.values.s;
        gc_state.modal.spindle_mode = SPINDLE_RPM_MODE;
        gc_state.spindle_limit = 0;
    }
    else if(gc_block.modal.spindle_mode == SPINDLE_SURFACE_MODE)
    {
        // Set spindle max rpm for G96
        gc_state.spindle_limit = gc_block.values.d;

        // Set surface speed
        gc_state.spindle_speed = gc_block.values.s;

        // Set mode
        gc_state.modal.spindle_mode = SPINDLE_SURFACE_MODE;
    }

    // NOTE: Pass zero spindle speed for all restricted laser motions.
    if(BIT_IS_FALSE(gc_parser_flags, GC_PARSER_LASER_DISABLE))
    {
        pl_data->spindle_speed = gc_state.spindle_speed; // Record data for planner use.
    }
    // else { pl_data->spindle_speed = 0.0; } // Initialized as zero already.

    // [6. Change tool ]: M6
    if(change_tool && (settings.tool_change > 0))
    {
        if(sys.is_homed)
        {
            TC_ChangeCurrentTool();
        }
        else
        {
            return STATUS_MACHINE_NOT_HOMED;
        }
    }
    if(apply_tool && (settings.tool_change == 3))
    {
        if(sys.is_homed)
        {
            TC_ApplyToolOffset();
        }
        else
        {
            return STATUS_MACHINE_NOT_HOMED;
        }
    }

    // [Input Output Control ]:
    if(io_cmd > 0)
    {
        uint8_t io_on = 0;

        switch (io_cmd)
        {
        case 62:
            io_on = 1;
        case 63:
            // Syncronized
            Protocol_BufferSynchronize();

            if(io_on)
            {
                // Switch IO P on
            }
            else
            {
                // Switch IO P off
            }
            break;

        case 64:
            io_on = 1;
        case 65:
            if(io_on)
            {
                // Switch IO P on
            }
            else
            {
                // Switch IO P off
            }
            break;

        case 66:

            break;

        default:
            break;
        }
    }

    // [7. Spindle control ]:
    if(gc_state.modal.spindle != gc_block.modal.spindle)
    {
        // Update spindle control and apply spindle speed when enabling it in this block.
        // NOTE: All spindle state changes are synced, even in laser mode. Also, pl_data,
        // rather than gc_state, is used to manage laser state for non-laser motions.
        Spindle_Sync(gc_block.modal.spindle, pl_data->spindle_speed);
        gc_state.modal.spindle = gc_block.modal.spindle;
    }

    // Set condition flag for planner use.
    pl_data->condition |= gc_state.modal.spindle;

    // [8. Coolant control ]:
    if (gc_state.modal.coolant != gc_block.modal.coolant)
    {
        // NOTE: Coolant M-codes are modal. Only one command per line is allowed. But, multiple states
        // can exist at the same time, while coolant disable clears all states.
        Coolant_Sync(gc_block.modal.coolant);
        gc_state.modal.coolant = gc_block.modal.coolant;
    }
    // Set condition flag for planner use.
    pl_data->condition |= gc_state.modal.coolant;

    // [9. Override control ]: NOT SUPPORTED. Always enabled. Except for a Grbl-only parking control.
#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
    if(gc_state.modal.override != gc_block.modal.override)
    {
        gc_state.modal.override = gc_block.modal.override;
        MC_OverrideCtrlUpdate(gc_state.modal.override);
    }
#endif

    // [10. Dwell ]:
    if(gc_block.non_modal_command == NON_MODAL_DWELL)
    {
        MC_Dwell(gc_block.values.p);
    }

    // [11. Set active plane ]:
    gc_state.modal.plane_select = gc_block.modal.plane_select;

    // [12. Set length units ]:
    gc_state.modal.units = gc_block.modal.units;

    // [13. Cutter radius compensation ]: G41/42 NOT SUPPORTED
    // gc_state.modal.cutter_comp = gc_block.modal.cutter_comp; // NOTE: Not needed since always disabled.

    // [14. Cutter length compensation ]: G43.1 and G49 supported. G43 NOT SUPPORTED.
    // NOTE: If G43 were supported, its operation wouldn't be any different from G43.1 in terms
    // of execution. The error-checking step would simply load the offset value into the correct
    // axis of the block XYZ value array.
    if(axis_command == AXIS_COMMAND_TOOL_LENGTH_OFFSET)   // Indicates a change.
    {
        gc_state.modal.tool_length = gc_block.modal.tool_length;
        if (gc_state.modal.tool_length == TOOL_LENGTH_OFFSET_CANCEL)   // G49
        {
            gc_block.values.xyz[TOOL_LENGTH_OFFSET_AXIS] = 0.0;

            for (uint8_t i = 0; i < N_AXIS; i++)
            {
                gc_state.tool_length_offset[i] = 0.0;
            }
        }
        // G43.1
        if ((update_tooltable == 0) && gc_state.tool_length_offset_dynamic[TOOL_LENGTH_OFFSET_AXIS] != gc_block.values.xyz[TOOL_LENGTH_OFFSET_AXIS])
        {
            gc_state.tool_length_offset_dynamic[TOOL_LENGTH_OFFSET_AXIS] = gc_block.values.xyz[TOOL_LENGTH_OFFSET_AXIS];
            System_FlagWcoChange();
        }
        // G43
        if(update_tooltable == 1)
        {
            TC_ApplyToolOffset();
        }
    }

    // [15. Coordinate system selection ]:
    if(gc_state.modal.coord_select != gc_block.modal.coord_select)
    {
        gc_state.modal.coord_select = gc_block.modal.coord_select;
        memcpy(gc_state.coord_system, block_coord_system, N_AXIS*sizeof(float));
        System_FlagWcoChange();
    }

    // [16. Set path control mode ]: G61.1/G64 NOT SUPPORTED
    // gc_state.modal.control = gc_block.modal.control; // NOTE: Always default.

    // [17. Set distance mode ]:
    gc_state.modal.distance = gc_block.modal.distance;

    // [18. Set retract mode ]:
    gc_state.modal.retract = gc_block.modal.retract;

    // [19. Go to predefined position, Set G10, or Set axis offsets ]:
    switch(gc_block.non_modal_command)
    {
    case NON_MODAL_SET_COORDINATE_DATA:
        Settings_WriteCoordData(coord_select, gc_block.values.ijk);
        // Update system coordinate system if currently active.
        if(gc_state.modal.coord_select == coord_select)
        {
            memcpy(gc_state.coord_system, gc_block.values.ijk, N_AXIS*sizeof(float));
            System_FlagWcoChange();
        }
        break;

    case NON_MODAL_GO_HOME_0:
    case NON_MODAL_GO_HOME_1:
        // Move to intermediate position before going home. Obeys current coordinate system and offsets
        // and absolute and incremental modes.
        // Set rapid motion condition flag.
        pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
        if(axis_command)
        {
            MC_Line(gc_block.values.xyz, pl_data);
        }

        MC_Line(gc_block.values.ijk, pl_data);
        memcpy(gc_state.position, gc_block.values.ijk, N_AXIS*sizeof(float));
        break;

    case NON_MODAL_SET_HOME_0:
        Settings_WriteCoordData(SETTING_INDEX_G28, gc_state.position);
        break;

    case NON_MODAL_SET_HOME_1:
        Settings_WriteCoordData(SETTING_INDEX_G30, gc_state.position);
        break;

    case NON_MODAL_SET_COORDINATE_OFFSET:
        memcpy(gc_state.coord_offset, gc_block.values.xyz, sizeof(gc_block.values.xyz));
        System_FlagWcoChange();
        break;

    case NON_MODAL_RESET_COORDINATE_OFFSET:
        // Disable G92 offsets by zeroing offset vector.
        clear_vector(gc_state.coord_offset);
        System_FlagWcoChange();
        break;
    }


    // [20. Motion modes ]:
    // NOTE: Commands G10,G28,G30,G92 lock out and prevent axis words from use in motion modes.
    // Enter motion modes only if there are axis words or a motion mode command word in the block.
    gc_state.modal.motion = gc_block.modal.motion;
    if(gc_state.modal.motion != MOTION_MODE_NONE)
    {
        if(axis_command == AXIS_COMMAND_MOTION_MODE)
        {
            uint8_t gc_update_pos = GC_UPDATE_POS_TARGET;
            if(gc_state.modal.motion == MOTION_MODE_LINEAR)
            {
                MC_Line(gc_block.values.xyz, pl_data);
            }
            else if(gc_state.modal.motion == MOTION_MODE_SEEK)
            {
                // Set rapid motion condition flag.
                pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                MC_Line(gc_block.values.xyz, pl_data);
            }
            else if((gc_state.modal.motion == MOTION_MODE_CW_ARC) || (gc_state.modal.motion == MOTION_MODE_CCW_ARC))
            {
                MC_Arc(gc_block.values.xyz, pl_data, gc_state.position, gc_block.values.ijk, gc_block.values.r,
                       axis_0, axis_1, axis_linear, BIT_IS_TRUE(gc_parser_flags, GC_PARSER_ARC_IS_CLOCKWISE));
            }
            else if (gc_state.modal.motion == MOTION_MODE_DRILL || gc_state.modal.motion == MOTION_MODE_DRILL_DWELL ||
                    gc_state.modal.motion == MOTION_MODE_DRILL_PECK || gc_state.modal.motion == MOTION_MODE_DRILL_BREAK)
            {
                float xyz[N_AXIS] = {0.0};
                float clear_z = gc_block.values.r + gc_state.coord_system[Z_AXIS] + gc_state.coord_offset[Z_AXIS];
                float delta_x = 0.0;
                float delta_y = 0.0;

                if(gc_state.modal.distance == DISTANCE_MODE_INCREMENTAL)
                {
                    clear_z += old_xyz[Z_AXIS];
                    gc_block.values.xyz[Z_AXIS] = clear_z + (gc_block.values.xyz[Z_AXIS] - old_xyz[Z_AXIS]);

                    delta_x = gc_block.values.xyz[X_AXIS] - old_xyz[X_AXIS];
                    delta_y = gc_block.values.xyz[Y_AXIS] - old_xyz[Y_AXIS];
                }
                else
                {
                    clear_z += gc_state.tool_length_offset_dynamic[TOOL_LENGTH_OFFSET_AXIS] + gc_state.tool_length_offset[TOOL_LENGTH_OFFSET_AXIS];
                }

                if(clear_z < gc_block.values.xyz[Z_AXIS])
                {
                    // Error
                    return STATUS_GCODE_INVALID_TARGET;
                }

                //-- [G81 - G83] --//

                // 0. Check if old_z < clear_z
                if(old_xyz[Z_AXIS] < clear_z)
                {
                    // Move old_z to clear_z
                    memcpy(xyz, old_xyz, N_AXIS*sizeof(float));
                    xyz[Z_AXIS] = clear_z;

                    // Set rapid motion condition flag.
                    pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                    MC_Line(xyz, pl_data);
                }
                else
                {
                    // Reset z to old/current z
                    xyz[Z_AXIS] = old_xyz[Z_AXIS];
                }

                if(gc_block.values.l == 0)
                {
                    // Force at least one iteration of loop
                    gc_block.values.l = 1;
                }

                for(uint8_t repeat = 0; repeat < gc_block.values.l; repeat++)
                {
                    // 1. Rapid move to XY (XY)
                    xyz[X_AXIS] = gc_block.values.xyz[X_AXIS] + (delta_x*repeat);
                    xyz[Y_AXIS] = gc_block.values.xyz[Y_AXIS] + (delta_y*repeat);
                    // Set rapid motion condition flag.
                    pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                    MC_Line(xyz, pl_data);

                    // 2. Rapid move to R (Z)
                    xyz[Z_AXIS] = clear_z;
                    // Set rapid motion condition flag.
                    pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                    MC_Line(xyz, pl_data);

                    if (gc_state.modal.motion == MOTION_MODE_DRILL || gc_state.modal.motion == MOTION_MODE_DRILL_DWELL)
                    {
                        //-- G81 -- G82 --//
                        // 3. Move the Z-axis at the current feed rate to the Z position.
                        // Clear rapid move
                        pl_data->condition &= ~PL_COND_FLAG_RAPID_MOTION;
                        xyz[Z_AXIS] = gc_block.values.xyz[Z_AXIS];
                        MC_Line(xyz, pl_data);
                    }
                    else    // Peck / Chip break
                    {
                        uint8_t exit = 0;
                        //-- G83/73 --//
                        for(float curr_z = clear_z - gc_block.values.q; exit == 0; curr_z -= gc_block.values.q)
                        {
                            // Check if target depth exceeds final depth
                            if(curr_z <= gc_block.values.xyz[Z_AXIS])
                            {
                                curr_z = gc_block.values.xyz[Z_AXIS];
                                exit = 1;
                            }

                            // Move the Z-axis at the current feed rate to the Z position.
                            // Clear rapid move
                            pl_data->condition &= ~PL_COND_FLAG_RAPID_MOTION;
                            xyz[Z_AXIS] = curr_z;
                            MC_Line(xyz, pl_data);

                            if(gc_state.modal.motion == MOTION_MODE_DRILL_PECK)
                            {
                                // Rapid move to R
                                xyz[Z_AXIS] = clear_z;
                                // Set rapid motion condition flag.
                                pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                                MC_Line(xyz, pl_data);
                            }
                            else
                            {
                                // Back off a bit
                                xyz[Z_AXIS] += 2;
                                // Set rapid motion condition flag.
                                pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                                MC_Line(xyz, pl_data);
                            }

                            if(exit == 0)
                            {
                                // Prepare next hole
                                // Rapid move to bottom of hole (backed off a bit)
                                xyz[Z_AXIS] = curr_z + 0.4;
                                // Set rapid motion condition flag.
                                pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                                MC_Line(xyz, pl_data);
                            }
                        }
                    }

                    if(gc_state.modal.motion == MOTION_MODE_DRILL_DWELL)
                    {
                        //-- G82 --//
                        MC_Dwell(gc_block.values.p);
                    }

                    // 4. The Z-axis does a rapid move to clear Z.
                    if((gc_state.modal.retract == RETRACT_OLD_Z) && clear_z < old_xyz[Z_AXIS])
                    {
                        //-- G98 --//
                        // Retract to OLD_Z
                        xyz[Z_AXIS] = old_xyz[Z_AXIS];
                    }
                    else
                    {
                        //-- G99 --//
                        // Retract to r
                        xyz[Z_AXIS] = clear_z;
                    }
                    // Set rapid motion condition flag.
                    pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                    MC_Line(xyz, pl_data);
                }
                // Update position
                memcpy(gc_block.values.xyz, xyz, N_AXIS*sizeof(float));
            }
            else if(gc_state.modal.motion == MOTION_MODE_SPINDLE_SYNC)
            {
                // Perform a small move towards target to trigger backlash compensation, because it could affect synchronized motion
                old_xyz[Z_AXIS] -= 0.001;
                MC_Line(old_xyz, pl_data);

                // Wait till everything is finished
                Protocol_BufferSynchronize();

                // Get current RPM
                uint16_t rpm = Spindle_GetRPM();
                pl_data->spindle_speed = rpm;
                if(rpm > 0)
                {
                    // Calculate feed rate depending on current RPM along Z-axis
                    pl_data->feed_rate = rpm * gc_block.values.ijk[Z_AXIS];

                    if(!isEqual_f(gc_block.values.xyz[X_AXIS], old_xyz[X_AXIS]))
                    {
                        // Also movement in X-axis
                        float f = sqrtf(powf(gc_block.values.xyz[X_AXIS], 2.0) + powf(gc_block.values.ijk[Z_AXIS], 2.0));

                        pl_data->feed_rate *= f;
                    }
                }
                else
                {
                    // Spindle not moving
                    return STATUS_IDLE_ERROR;
                }

                // Sync move
                MC_LineSync(gc_block.values.xyz, pl_data, gc_block.values.ijk[Z_AXIS]);
            }
            else if(gc_state.modal.motion == MOTION_MODE_THREADING)
            {
                float pitch = gc_block.values.p;
                float peak = gc_block.values.ijk[X_AXIS];
                float doc = gc_block.values.ijk[Y_AXIS];
                float final_depth = gc_block.values.ijk[Z_AXIS];
                float regression = min(gc_block.values.r, 6.0);
                uint8_t spring_passes = gc_block.values.h;
                float angle = gc_block.values.q;
                //float taper_dist = gc_block.values.e;
                //uint8_t taper_type = gc_block.values.l;

                float cur_xyz[N_AXIS];
                memcpy(cur_xyz, old_xyz, sizeof(cur_xyz));

                float next_doc = 0.0;
                uint8_t leave = 0;
                uint16_t idx = 0;

                // Calculate z offset for angled slide compensation
                float z_offset = doc * tanf(angle*M_PI/180.0);


                // Wait till everything is finished
                Protocol_BufferSynchronize();

                // Get current RPM
                uint16_t rpm = Spindle_GetRPM();
                pl_data->spindle_speed = rpm;
                if(rpm > 0)
                {
                    // Calculate feed rate depending on current RPM along Z-axis
                    pl_data->feed_rate = rpm * pitch;

                    if(!isEqual_f(gc_block.values.xyz[X_AXIS], old_xyz[X_AXIS]))
                    {
                        // Also movement in X-axis
                        float f = sqrtf(powf(gc_block.values.xyz[X_AXIS], 2.0) + powf(pitch, 2.0));

                        pl_data->feed_rate *= f;
                    }
                }
                else
                {
                    // Spindle not moving
                    return STATUS_IDLE_ERROR;
                }

                while((leave == 0) || (spring_passes != 0))
                {
                    if(leave == 0)
                    {
                        idx++;
                    }

                    if((leave == 1) && (spring_passes > 0))
                    {
                        // Thread cutting done, only spring passes left
                        spring_passes--;
                    }

                    // Perform a small move towards target to trigger backlash compensation, because it could affect synchronized motion
                    old_xyz[Z_AXIS] -= 0.001;
                    if(leave == 0)
                    {
                        // Add Z offset
                        old_xyz[Z_AXIS] -= z_offset;
                    }
                    MC_Line(old_xyz, pl_data);
                    old_xyz[Z_AXIS] += 0.001;

                    // Rapid from drive line to depth of cut
                    pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                    if(peak < 0.0)
                    {
                        // External thread
                        cur_xyz[X_AXIS] = old_xyz[X_AXIS] + peak - doc - next_doc;
                        if(cur_xyz[X_AXIS] <= (old_xyz[X_AXIS] + peak - final_depth))
                        {
                            // Limit to final depth
                            cur_xyz[X_AXIS] = old_xyz[X_AXIS] + peak - final_depth;
                            leave = 1;
                        }
                    }
                    else if(peak > 0.0)
                    {
                        // Internal thread
                        cur_xyz[X_AXIS] = old_xyz[X_AXIS] + peak + doc + next_doc;
                        if(cur_xyz[X_AXIS] <= (old_xyz[X_AXIS] + peak + final_depth))
                        {
                            // Limit to final depth
                            cur_xyz[X_AXIS] = old_xyz[X_AXIS] + peak + final_depth;
                            leave = 1;
                        }
                    }
                    else
                    {
                        // peak is zero
                        return STATUS_BAD_NUMBER_FORMAT;
                    }
                    MC_Line(cur_xyz, pl_data);

                    // Handle entry taper
                    // ToDo

                    Protocol_BufferSynchronize();

                    // Actual synced move
                    cur_xyz[Z_AXIS] = gc_block.values.xyz[Z_AXIS] - z_offset*(idx-1);
                    pl_data->condition &= ~PL_COND_FLAG_RAPID_MOTION;
                    // Sync move
                    MC_LineSync(cur_xyz, pl_data, pitch);

                    // Handle exit taper
                    // ToDo

                    // Rapid out to drive line
                    pl_data->condition |= PL_COND_FLAG_RAPID_MOTION;
                    cur_xyz[X_AXIS] = old_xyz[X_AXIS];
                    MC_Line(cur_xyz, pl_data);

                    // Cycle ends at end of drive line
                    if(leave == 0 || spring_passes != 0)
                    {
                        //Move back to start of drive line
                        MC_Line(old_xyz, pl_data);
                        cur_xyz[Z_AXIS] = old_xyz[Z_AXIS];
                    }

                    // Calculate DOC of next move
                    if(regression <= 1.0001)
                    {
                        // No regression defined
                        next_doc += doc;
                    }
                    else
                    {
                        // ToDo
                        next_doc += (1/idx) * doc;
                    }
                }
            }
            else
            {
                // NOTE: gc_block.values.xyz is returned from mc_probe_cycle with the updated position value. So
                // upon a successful probing cycle, the machine position and the returned value should be the same.
#ifndef ALLOW_FEED_OVERRIDE_DURING_PROBE_CYCLES
                pl_data->condition |= PL_COND_FLAG_NO_FEED_OVERRIDE;
#endif
                gc_update_pos = MC_ProbeCycle(gc_block.values.xyz, pl_data, gc_parser_flags);
            }

            // As far as the parser is concerned, the position is now == target. In reality the
            // motion control system might still be processing the action and the real tool position
            // in any intermediate location.
            if(gc_update_pos == GC_UPDATE_POS_TARGET)
            {
                // gc_state.position[] = gc_block.values.xyz[]
                memcpy(gc_state.position, gc_block.values.xyz, sizeof(gc_block.values.xyz));
            }
            else if (gc_update_pos == GC_UPDATE_POS_SYSTEM)
            {
                // gc_state.position[] = sys_position
                GC_SyncPosition();
            } // == GC_UPDATE_POS_NONE
        }
    }

    // [21. Program flow ]:
    // M0,M1,M2,M30: Perform non-running program flow actions. During a program pause, the buffer may
    // refill and can only be resumed by the cycle start run-time command.
    gc_state.modal.program_flow = gc_block.modal.program_flow;
    if(gc_state.modal.program_flow)
    {
        // Sync and finish all remaining buffered motions before moving on.
        Protocol_BufferSynchronize();

        if(gc_state.modal.program_flow == PROGRAM_FLOW_PAUSED)
        {
            if(sys.state != STATE_CHECK_MODE)
            {
                // Use feed hold for program pause.
                System_SetExecStateFlag(EXEC_FEED_HOLD);
                // Execute suspend.
                Protocol_ExecuteRealtime();
            }
        }
        else   // == PROGRAM_FLOW_COMPLETED
        {
            // Upon program complete, only a subset of g-codes reset to certain defaults, according to
            // LinuxCNC's program end descriptions and testing. Only modal groups [G-code 1,2,3,5,7,12]
            // and [M-code 7,8,9] reset to [G1,G17,G90,G94,G40,G54,M5,M9,M48]. The remaining modal groups
            // [G-code 4,6,8,10,13,14,15] and [M-code 4,5,6] and the modal words [F,S,T,H] do not reset.
            gc_state.modal.motion = MOTION_MODE_LINEAR;
            gc_state.modal.plane_select = PLANE_SELECT_XY;
            gc_state.modal.distance = DISTANCE_MODE_ABSOLUTE;
            gc_state.modal.feed_rate = FEED_RATE_MODE_UNITS_PER_MIN;
            // gc_state.modal.cutter_comp = CUTTER_COMP_DISABLE; // Not supported.
            gc_state.modal.coord_select = 0; // G54
            gc_state.modal.spindle = SPINDLE_DISABLE;
            gc_state.modal.coolant = COOLANT_DISABLE;
#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
#ifdef DEACTIVATE_PARKING_UPON_INIT
            gc_state.modal.override = OVERRIDE_DISABLED;
#else
            gc_state.modal.override = OVERRIDE_PARKING_MOTION;
#endif
#endif

#ifdef RESTORE_OVERRIDES_AFTER_PROGRAM_END
            sys.f_override = DEFAULT_FEED_OVERRIDE;
            sys.r_override = DEFAULT_RAPID_OVERRIDE;
            sys.spindle_speed_ovr = DEFAULT_SPINDLE_SPEED_OVERRIDE;
#endif

            // Execute coordinate change and spindle/coolant stop.
            if(sys.state != STATE_CHECK_MODE)
            {
                if(!(Settings_ReadCoordData(gc_state.modal.coord_select, gc_state.coord_system)))
                {
                    return STATUS_SETTING_READ_FAIL;
                }
                // Set to refresh immediately just in case something altered.
                System_FlagWcoChange();
                Spindle_SetState(SPINDLE_DISABLE, 0.0);
                Coolant_SetState(COOLANT_DISABLE);
            }
            // Reset tool change - May not be in accordance with LinuxCNC
            TC_Init();

            Report_FeedbackMessage(MESSAGE_PROGRAM_END);
        }
        // Reset program flow.
        gc_state.modal.program_flow = PROGRAM_FLOW_RUNNING;
    }

    // TODO: % to denote start of program.

    return STATUS_OK;
}


/*
  Not supported:

  - Tool radius compensation
  - Evaluation of expressions
  - Variables
  - Override control (TBD)
  - Switches

   (*) Indicates optional parameter, enabled through config.h and re-compile
   group 0 = {G92.2, G92.3} (Non modal: Cancel and re-enable G92 offsets)
   group 1 = {G81 - G89} (Motion modes: Canned cycles)
   group 4 = {M1} (Optional stop, ignored)
   group 6 = {M6} (Tool change)
   group 7 = {G41, G42} cutter radius compensation (G40 is supported)
   group 8 = {G43} tool length offset (G43.1/G49 are supported)
   group 8 = {M7*} enable mist coolant (* Compile-option)
   group 9 = {M48, M49, M56*} enable/disable override switches (* Compile-option)
   group 10 = {G98, G99} return mode canned cycles
   group 13 = {G61.1, G64} path control mode (G61 is supported)
*/
