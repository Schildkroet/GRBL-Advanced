/*
  Encoder.c - Encoder
  Part of Grbl-Advanced

  Copyright (c) 2021 Patrick F.

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
#include "Encoder.h"
#include "Config.h"
#include "TIM.h"


static uint32_t OvfCnt = 0;
static uint32_t CntValue = 0;

static uint16_t PulsesPerRev = 360;
static bool isZero = false;


void Encoder_Init(uint16_t ppr)
{
    TIM4_Init(ppr);
    Encoder_Reset();
    PulsesPerRev = ppr;
}


void Encoder_Reset(void)
{
    OvfCnt = 0;
    CntValue = 0;
    isZero = false;
}


void Encoder_SetPulsesPerRev(uint16_t ppr)
{
    Encoder_Init(ppr);
}


uint32_t Encoder_GetValue(void)
{
    return CntValue + TIM4_CNT();
}


void Encoder_SetValue(uint32_t val)
{
    CntValue = val - TIM4_CNT();
}


bool Encoder_Zero(void)
{
    if(isZero)
    {
        isZero = false;
        return true;
    }
    return false;
}


void Encoder_OvfISR(void)
{
    OvfCnt++;
    CntValue += PulsesPerRev;
    isZero = true;
}
