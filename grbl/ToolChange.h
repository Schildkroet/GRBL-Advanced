/*
  ToolChange.h - Changing tool
  Part of Grbl-Advanced

  Copyright (c)	2018-2019 Patrick F.

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

#ifndef TOOLCHANGE_H_INCLUDED
#define TOOLCHANGE_H_INCLUDED


#include <stdint.h>


void TC_Init(void);
void TC_ChangeCurrentTool(void);
void TC_ProbeTLS(void);


#endif /* TOOLCHANGE_H_INCLUDED */
