/*
  protocol.h - controls Grbl execution protocol and procedures
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
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
#ifndef PROTOCOL_H
#define PROTOCOL_H


// Starts Grbl main loop. It handles all incoming characters from the serial port and executes
// them as they complete. It is also responsible for finishing the initialization procedures.
void Protocol_MainLoop(void);

// Checks and executes a realtime command at various stop points in main program
void Protocol_ExecuteRealtime(void);

void Protocol_ExecRtSystem(void);

// Executes the auto cycle feature, if enabled.
void Protocol_AutoCycleStart(void);

// Block until all buffered steps are executed
void Protocol_BufferSynchronize(void);


#endif // PROTOCOL_H
