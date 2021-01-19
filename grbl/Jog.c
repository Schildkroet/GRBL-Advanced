/*
  Jog.h - Jogging methods
  Part of Grbl-Advanced

  Copyright (c) 2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2017 Patrick F.

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
#include "Planner.h"
#include "Jog.h"
#include "Settings.h"
#include "Report.h"
#include "System.h"
#include "MotionControl.h"
#include "Stepper.h"


// Sets up valid jog motion received from g-code parser, checks for soft-limits, and executes the jog.
uint8_t Jog_Execute(Planner_LineData_t *pl_data, Parser_Block_t *gc_block)
{
    // Initialize planner data struct for jogging motions.
    // NOTE: Spindle and coolant are allowed to fully function with overrides during a jog.
    pl_data->feed_rate = gc_block->values.f;
    pl_data->condition |= PL_COND_FLAG_NO_FEED_OVERRIDE;
    pl_data->line_number = gc_block->values.n;

    if(BIT_IS_TRUE(settings.flags, BITFLAG_SOFT_LIMIT_ENABLE))
    {
        if(System_CheckTravelLimits(gc_block->values.xyz))
        {
            return STATUS_TRAVEL_EXCEEDED;
        }
    }

    // Valid jog command. Plan, set state, and execute.
    MC_Line(gc_block->values.xyz, pl_data);
    if(sys.state == STATE_IDLE)
    {
        if (Planner_GetCurrentBlock() != 0)   // Check if there is a block to execute.
        {
            sys.state = STATE_JOG;
            Stepper_PrepareBuffer();
            Stepper_WakeUp();  // NOTE: Manual start. No state machine required.
        }
    }

    return STATUS_OK;
}
