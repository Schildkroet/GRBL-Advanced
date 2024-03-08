/*
  Nvm.c - Non-volatile-memory interface.
  Part of Grbl-Advanced

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c) 2019-2024 Patrick F.

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
#include "Nvm.h"
#include "Config.h"
#include "M24C0X.h"
#include "System32.h"
#include "eeprom.h"


void Nvm_Init(void)
{
#if (USE_EXT_EEPROM)
    M24C0X_Init();
#else
    EE_Init();
#endif
}


uint8_t Nvm_ReadByte(uint16_t Address)
{
#if (USE_EXT_EEPROM)
    return M24C0X_ReadByte(Address);
#else
    return EE_ReadByte(Address);
#endif
}


void Nvm_WriteByte(uint16_t Address, uint8_t Data)
{
#if (USE_EXT_EEPROM)
    uint8_t ret = 1;
    uint8_t timeout = 8;

    while (ret > 0 && timeout > 0)
    {
        ret = M24C0X_WriteByte(Address, Data);
        timeout--;
        Delay_ms(1);
    }
#else
    EE_WriteByte(Address, Data);
#endif
}


uint8_t Nvm_Read(uint8_t *DataOut, uint16_t Address, uint16_t size)
{
#if (USE_EXT_EEPROM)
    return M24C0X_ReadByteArray(Address, DataOut, size);
#else
    return EE_ReadByteArray(DataOut, Address, size);
#endif
}


uint8_t Nvm_Write(uint16_t Address, const uint8_t *DataIn, uint16_t size)
{
#if (USE_EXT_EEPROM)
    uint8_t ret = 1;
    uint8_t timeout = 8;

    while(ret > 0 && timeout > 0)
    {
        ret = M24C0X_WriteByteArray(Address, (uint8_t*)DataIn, size);
        timeout--;
        Delay_ms(1);
    }
    return ret;
#else
    EE_WriteByteArray(Address, DataIn, size);
    return 0;
#endif
}


void Nvm_Update(void)
{
#if (USE_EXT_EEPROM)
    // Do nothing
#else
    EE_Program();
#endif
}
