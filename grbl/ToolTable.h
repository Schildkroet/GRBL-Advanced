/*
  ToolTable.h - Tool Table Library
  Part of Grbl-Advanced

  Copyright (c)	2020 Patrick F.

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
#ifndef TOOLTABLE_H_INCLUDED
#define TOOLTABLE_H_INCLUDED

#include <stdint.h>


#define MAX_TOOL_NR     20


#pragma pack(push, 1) // exact fit - no padding
typedef struct
{
    float x_offset;
    float y_offset;
    float z_offset;
    float reserved;
} ToolParams_t;


typedef struct
{
    ToolParams_t tools[MAX_TOOL_NR];
} ToolTable_t;
#pragma pack(pop)


void TT_Init(void);
void TT_Reset(void);

void TT_GetToolParams(uint8_t tool_nr, ToolParams_t *params);
void TT_SaveToolParams(uint8_t tool_nr, ToolParams_t *params);


#endif /* TOOLTABLE_H_INCLUDED */
