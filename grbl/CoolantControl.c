/*
  CoolantControl.c - coolant control methods
  Part of Grbl-Advanced

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
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
#include "System.h"
#include "Protocol.h"
#include "GPIO.h"
#include "GCode.h"
#include "CoolantControl.h"
#include "Config.h"
#include "Settings.h"


void Coolant_Init(void)
{
    GPIO_InitGPIO(GPIO_COOLANT);
    Coolant_Stop();
}


// Directly called by coolant_init(), coolant_set_state(), and mc_reset(), which can be at
// an interrupt-level. No report flag set, but only called by routines that don't need it.
void Coolant_Stop(void)
{
    if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_FLOOD_PIN))
    {
        GPIO_SetBits(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
    }
    else
    {
        GPIO_ResetBits(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
    }

    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_ENABLE_M7))
    {
        if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_MIST_PIN))
        {
            GPIO_SetBits(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);
        }
        else
        {
            GPIO_ResetBits(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);
        }
    }
}

// Returns current coolant output state. Overrides may alter it from programmed state.
uint8_t Coolant_GetState(void)
{
    uint8_t cl_state = COOLANT_STATE_DISABLE;

    uint8_t flood_state = GPIO_ReadInputDataBit(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
    uint8_t mist_state = GPIO_ReadInputDataBit(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);

    if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_FLOOD_PIN))
    {
        flood_state = !flood_state;
    }
    if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_MIST_PIN))
    {
        mist_state = !mist_state;
    }

    // TODO: Check if reading works
    if (flood_state)
    {
        cl_state |= COOLANT_STATE_FLOOD;
    }

    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_ENABLE_M7))
    {
        if (mist_state)
        {
            cl_state |= COOLANT_STATE_MIST;
        }
    }

    return cl_state;
}


// Main program only. Immediately sets flood coolant running state and also mist coolant,
// if enabled. Also sets a flag to report an update to a coolant state.
// Called by coolant toggle override, parking restore, parking retract, sleep mode, g-code
// parser program end, and g-code parser coolant_sync().
void Coolant_SetState(const uint8_t mode)
{
    if(sys.abort)
    {
        // Block during abort.
        return;
    }

    if (mode & COOLANT_FLOOD_ENABLE)
    {
        if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_FLOOD_PIN))
        {
            GPIO_ResetBits(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
        }
        else
        {
            GPIO_SetBits(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
        }
    }
    else
    {
        if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_FLOOD_PIN))
        {
            GPIO_SetBits(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
        }
        else
        {
            GPIO_ResetBits(GPIO_COOL_FLOOD_PORT, GPIO_COOL_FLOOD_PIN);
        }
    }

    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_ENABLE_M7))
    {
        if (mode & COOLANT_MIST_ENABLE)
        {
            if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_MIST_PIN))
            {
                GPIO_ResetBits(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);
            }
            else
            {
                GPIO_SetBits(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);
            }
        }
        else
        {
            if (BIT_IS_TRUE(settings.input_invert_mask, BITFLAG_INVERT_MIST_PIN))
            {
                GPIO_SetBits(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);
            }
            else
            {
                GPIO_ResetBits(GPIO_COOL_MIST_PORT, GPIO_COOL_MIST_PIN);
            }
        }
    }

    sys.report_ovr_counter = 0; // Set to report change immediately
}

// G-code parser entry-point for setting coolant state. Forces a planner buffer sync and bails
// if an abort or check-mode is active.
void Coolant_Sync(const uint8_t mode)
{
    if(sys.state == STATE_CHECK_MODE)
    {
        return;
    }

    // Ensure coolant turns on when specified in program.
    Protocol_BufferSynchronize();
    Coolant_SetState(mode);
}
