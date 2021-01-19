/*
  eeprom.h - Eeprom Header
  Part of STM32F4_HAL

  Copyright (c)	2017 Patrick F.

  STM32F4_HAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  STM32F4_HAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with STM32F4_HAL.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EEPROM_H
#define EEPROM_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f4xx.h"


#ifdef USE_EXT_EEPROM
    #define EEPROM_SIZE			1
#else
    #define EEPROM_SIZE			1024
#endif

/* Device voltage range supposed to be [2.7V to 3.6V], the operation will be done by word  */
#define VOLTAGE_RANGE			(uint8_t)VoltageRange_3

/* EEPROM start address in Flash */
#define EEPROM_START_ADDRESS	((uint32_t)0x08060000) /* EEPROM emulation start address (last sector): 384 Kb */

#define FLASH_SECTOR			FLASH_Sector_7


void EE_Init(void);

uint8_t EE_ReadByte(uint16_t VirtAddress);
void EE_WriteByte(uint16_t VirtAddress, uint8_t Data);

uint8_t EE_ReadByteArray(uint8_t *DataOut, uint16_t VirtAddress, uint16_t size);
void EE_WriteByteArray(uint16_t VirtAddress, uint8_t *DataIn, uint16_t size);

void EE_Program(void);
void EE_Erase(void);


#endif /* EEPROM_H */
