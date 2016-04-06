// This file is part of UVExposureBox.
//
// Copyright (C) 2016 Kurt Skauen <http://kavionic.com/>
//
// UVExposureBox is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// UVExposureBox is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with UVExposureBox. If not, see < http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////


#ifndef __PWMCONTROLLER_H__
#define __PWMCONTROLLER_H__

#include "Hardware.h"

class PWMController
{
public:

    static void    SetLCDBacklight(uint8_t intensity);
    static uint8_t GetLCDBacklight();

    static void    SetLCDContrast(uint8_t intensity);
    static uint8_t GetLCDContrast();
    
    static void     SetLightPanel(LightPanelID::Enum panelID, uint8_t intensity);
    static uint8_t  GetLightPanel(LightPanelID::Enum panelID);
    static uint32_t GetLightPanelStartTime(LightPanelID::Enum panelID, uint8_t* power);
    
    static void     SetCompressor(uint8_t intensity);
    static uint8_t  GetCompressor();
    static uint32_t GetCompressorStartTime(uint8_t* power);

private:
    static uint32_t s_PanelStartTime[LightPanelID::e_Count];
    static uint32_t s_CompressorStartTime;
};

#endif //__PWMCONTROLLER_H__
