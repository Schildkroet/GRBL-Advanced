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
#ifndef JOG_H
#define JOG_H

#include <stdint.h>
#include "GCode.h"
#include "Planner.h"


// System motion line numbers must be zero.
#define JOG_LINE_NUMBER     0


// Sets up valid jog motion received from g-code parser, checks for soft-limits, and executes the jog.
uint8_t Jog_Execute(Planner_LineData_t *pl_data, Parser_Block_t *gc_block);


#endif // JOG_H
