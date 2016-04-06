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


#ifndef __RUNTIMETRACKER_H__
#define __RUNTIMETRACKER_H__

#include <avr/io.h>
#include "Hardware.h"

class RuntimeTracker
{
public:
    RuntimeTracker();
    
    void Restart();
    void Update(bool force);
    
    uint32_t GetTotalTime() const;
    void     AddTotalTime(uint16_t seconds);

    uint32_t GetPanelTime(LightPanelID::Enum panelID, bool normalized) const;
    void     AddPanelTime(LightPanelID::Enum panelID, uint16_t seconds, uint8_t power);

    uint32_t GetCompressorTime(bool normalized) const;
    void     AddCompressorTime(uint16_t seconds, uint8_t power);
    
private:
    static const uint32_t SAVE_TIME_INTERVAL_S = 10*60;
    static const uint32_t SAVE_TIME_INTERVAL_MS = SAVE_TIME_INTERVAL_S*1000;
    static uint32_t FindMostRecentTime(const uint32_t* array, uint8_t arraySize);
    static void     UpdateTime(uint32_t* array, uint8_t arraySize, uint32_t value);

    uint32_t m_LastUpdateTime;
    uint32_t m_LastSaveTime;
    uint8_t  m_UpdateSequence;
    uint32_t m_TotalTime;
    uint32_t m_PanelTime[LightPanelID::e_Count];
    uint32_t m_PanelNormalizedTime[LightPanelID::e_Count];
    uint32_t m_CompressorTime;
    uint32_t m_CompressorNormalizedTime;
    
};

#endif //__RUNTIMETRACKER_H__
