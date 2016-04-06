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

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdio.h>

#include "MenuAbout.h"
#include "Hardware.h"
#include "GlobalState.h"
#include "RuntimeTracker.h"
#include "Misc/Display.h"
#include "Misc/Utils.h"

extern int g_RuntimeWriteCount;

void MenuAbout::Initialize()
{
    m_ScrollOffset = 0;
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("UV 1288 (%d)"), g_RuntimeWriteCount);
//    g_Display.SetCursor(0, 1);
    Print();
}

static void PrintTime(const char* label, uint32_t time)
{
    uint8_t seconds = time % 60;
    uint32_t minutes = time / 60;
    uint32_t hours = minutes / 60;
    minutes %= 60;
    
    printf_P(PSTR("%S %lu:%02lu:%02u"), label, hours, minutes, seconds );
    g_Display.ClearToEndOfLine();
}

void MenuAbout::Print()
{
    for ( int8_t i = 0 ; i < 3 ; ++i ) {
        g_Display.SetCursor(0, i + 1);
        PrintLine(m_ScrollOffset + i);
    }
}
void MenuAbout::PrintLine(int8_t line)
{
    const RuntimeTracker& tracker = GlobalState::GetRuntimeTracker();
    switch(line)
    {
        case 0: PrintTime(PSTR("Total:   "), tracker.GetTotalTime());                             break;
        case 1: PrintTime(PSTR("Lower:   "), tracker.GetPanelTime(LightPanelID::e_Lower, false)); break;
        case 2: PrintTime(PSTR("Lower(N):"), tracker.GetPanelTime(LightPanelID::e_Lower, true));  break;
        case 3: PrintTime(PSTR("Upper:   "), tracker.GetPanelTime(LightPanelID::e_Upper, false)); break;
        case 4: PrintTime(PSTR("Upper(N):"), tracker.GetPanelTime(LightPanelID::e_Upper, true));  break;
        case 5: PrintTime(PSTR("Compr:   "), tracker.GetCompressorTime(false));                   break;
        case 6: PrintTime(PSTR("Compr(N):"), tracker.GetCompressorTime(true));                    break;
    }        
}

bool MenuAbout::Run(const Event& event)
{
    if ( event.type == EventID::e_KeyDown )
    {
        switch(event.value)
        {
            case KbdKeyCodes::e_Back:
                GlobalState::SetPrevState();
                break;
        }
    }
    else if ( event.type == EventID::e_KnobTurned )
    {
        m_ScrollOffset = clamp(0, 4, m_ScrollOffset + event.value);
        Print();
    }
    else if ( event.type == EventID::e_TimeTick1000 )
    {
        g_Display.SetCursor(0, 0);
        printf_P(PSTR("UV 1288 (%d)"), g_RuntimeWriteCount);
        Print();
    }        
    return false;
}