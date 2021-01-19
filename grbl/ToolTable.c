/*
  ToolTable.c - Tool Table Library
  Part of Grbl-Advanced

  Copyright (c) 2020 Patrick F.

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
#include "ToolTable.h"
#include "Settings.h"
#include "Report.h"
#include <string.h>


static ToolTable_t tool_table = {0};


void TT_Init(void)
{
    for(uint8_t i = 0; i < MAX_TOOL_NR; i++)
    {
        tool_table.tools[i].x_offset = 0.0;
        tool_table.tools[i].y_offset = 0.0;
        tool_table.tools[i].z_offset = 0.0;
        tool_table.tools[i].reserved = 0.0;
    }

    Settings_ReadToolTable(&tool_table);
}


void TT_Reset(void)
{
    for(uint8_t i = 0; i < MAX_TOOL_NR; i++)
    {
        tool_table.tools[i].x_offset = 0.0;
        tool_table.tools[i].y_offset = 0.0;
        tool_table.tools[i].z_offset = 0.0;
        tool_table.tools[i].reserved = 0.0;
    }

    Settings_StoreToolTable(&tool_table);
}


void TT_GetToolParams(uint8_t tool_nr, ToolParams_t *params)
{
    if(tool_nr < MAX_TOOL_NR)
    {
        memcpy(params, &tool_table.tools[tool_nr], sizeof(ToolParams_t));
    }
    else
    {
        Report_FeedbackMessage(MESSAGE_INVALID_TOOL);
    }
}


void TT_SaveToolParams(uint8_t tool_nr, ToolParams_t *params)
{
    if(tool_nr < MAX_TOOL_NR)
    {
        memcpy(&tool_table.tools[tool_nr], params, sizeof(ToolParams_t));
        Settings_StoreToolParams(tool_nr, params);
    }
    else
    {
        Report_FeedbackMessage(MESSAGE_INVALID_TOOL);
    }
}
