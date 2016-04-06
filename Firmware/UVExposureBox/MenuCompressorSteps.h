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


#ifndef __MENUCOMPRESSORSTEPS_H__
#define __MENUCOMPRESSORSTEPS_H__

#include "Hardware.h"

struct Event;

class MenuCompressorSteps
{
public:
    void Initialize();
    void Shutdown() {}
    bool Run(const Event& event);
    
private:
    uint8_t GetValue(uint8_t index) const;
    void    PrintLine(uint8_t line);
    void    Print();
    void    Flash();
    
    bool AddStep();
    bool DeleteStep();
    
    bool   m_EditMode:1;
    bool   m_RunCompressor:1;
    int8_t m_StepCount;
    int8_t m_Selection;
    int8_t m_Speeds[COMPRESSOR_SPEED_STEPS];
    int8_t m_FlashCount;
    uint32_t m_NextFlashTime;
};

#endif //__MENUCOMPRESSORSTEPS_H__
