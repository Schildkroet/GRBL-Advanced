/*
  Nvm.h - Non-volatile-memory interface.
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c)	2019 Patrick F.

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
#ifndef NVM_H_INCLUDED
#define NVM_H_INCLUDED


#include <stdint.h>


#define NVM_SIZE				1024


void Nvm_Init(void);

uint8_t Nvm_ReadByte(uint16_t Address);
void Nvm_WriteByte(uint16_t Address, uint8_t Data);

uint8_t Nvm_Read(uint8_t *DataOut, uint16_t Address, uint16_t size);
uint8_t Nvm_Write(uint16_t Address, uint8_t *DataIn, uint16_t size);

void Nvm_Update(void);


#endif /* NVM_H_INCLUDED */
