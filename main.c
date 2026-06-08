/*
  main.c - An embedded CNC Controller with rs274/ngc (g-code) support
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c)	2018-2024 Patrick F.

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
#include <stdint.h>

#include "CRC.h"
#include "GrIP.h"
#include "ServerTCP.h"
#include "System32.h"
#include "grbl_advance.h"
#include "Print.h"
#include "FIFO_USART.h"
#include "Platform.h"


int main(void)
{
    // Init formatted output
    Printf_Init();

    // Init CRC module
    CRC_Init();

    // Initialize GrIP protocol
    GrIP_Init();

    Settings_Init();
    System_Init();

    Stepper_Init();
    Limits_Init();

    System_ResetPosition();

#if (USE_ETH_IF)
    // Initialize TCP server
    ServerTCP_Init(ETH_SOCK, ETH_PORT);
#endif

    if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_HOMING_INIT_LOCK) && BIT_IS_TRUE(settings.flags, BITFLAG_HOMING_ENABLE))
    {
        sys.state = STATE_ALARM;
    }
    else if (BIT_IS_TRUE(settings.flags_ext, BITFLAG_FORCE_INITIALIZATION_ALARM))
    {
        sys.state = STATE_ALARM;
    }
    else
    {
        sys.state = STATE_IDLE;
    }

    // Init SysTick 1ms
    SysTick_Init();

    // Grbl-Advanced initialization loop upon power-up or a system abort. For the latter, all processes
    // will return to this loop to be cleanly re-initialized.
    while(1)
    {
        // Reset system variables.
        uint16_t prior_state = sys.state;
        uint8_t home_state = sys.is_homed;

        System_Clear();
        sys.state = prior_state;
        sys.is_homed = home_state;

        Probe_Reset();

        sys_probe_state = 0;
        sys_rt_exec_state = 0;
        sys_rt_exec_alarm = 0;
        sys_rt_exec_motion_override = 0;
        sys_rt_exec_accessory_override = 0;

        // Clear serial buffer to prevent undefined behavior
        FifoUsart_Init();

        // Reset Grbl-Advanced primary systems.
        GC_Init();
        Planner_Init();
        MC_Init();
        TC_Init();

        Coolant_Init();
        Limits_Init();
        Probe_Init();
        Spindle_Init();
        Stepper_Reset();

        // Sync cleared gcode and planner positions to current system position.
        Planner_SyncPosition();
        GC_SyncPosition();

        // Print welcome message. Indicates an initialization has occured at power-up or with a reset.
        Report_InitMessage();

        //--------------------------------------------------------------------------------//
        //-- Start Grbl-Advanced main loop. Processes program inputs and executes them. --//
        Protocol_MainLoop();
        //--------------------------------------------------------------------------------//
    }

    return 0;
}
