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


#ifndef __PANELCONTROLLER_H__
#define __PANELCONTROLLER_H__

#include "Hardware.h"

namespace PanelState
{
    enum Enum
    {
        e_Idle,
        e_WaitingToStart,
        e_Radiating,
        e_Paused,
        e_WaitingToStopCompressor
    };
}

class PanelController
{
public:
    static PanelState::Enum GetState() { return s_State; }
    static void StartRadiation();

    static void SkipStartupDelay();
    static void SkipCompressorStop(bool stopCompressor);

    static void PauseRadiation(bool pause);
    static void StopRadiation();
    
    
    static uint8_t GetRemainingStartupDelay() { return s_StartDelay; }
    static uint8_t GetRemainingCompressorStopDelay() { return s_CompressorStopDelay; }
        
    static bool GetRadiationTimeLeft(int16_t* lowerPanel, int16_t* upperPanel, int16_t* msTime = nullptr, int16_t* totalTime = nullptr);
    static uint8_t GetRadiationPower(LightPanelID::Enum panelID);
    
    static void UpdateRadiation();

private:
    static PanelState::Enum s_State;
    static uint16_t s_RadiationTimeMS;
    static int8_t   s_StartDelay;
    static int8_t   s_CompressorStopDelay;
    static int16_t  s_TotalTime;
    static int16_t  s_PanelTimeLeft[LightPanelID::e_Count];
    static uint8_t  s_PanelPower[LightPanelID::e_Count];
};

#endif //__PANELCONTROLLER_H__
