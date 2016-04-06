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
#include <stdio.h>

#include "MenuCalibration.h"
#include "Hardware.h"
#include "Keyboard.h"
#include "GlobalState.h"
#include "Config.h"

#include "Misc/Display.h"



void MenuCalibration::Initialize()
{
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("Calibrating panels."));

    m_Value = Config::GetPanelCalibration();
    
    Print();
}

void MenuCalibration::Print()
{
    g_Display.SetCursor(0, 1);

    int8_t upper = Config::GetPanelCalibration(LightPanelID::e_Upper, m_Value);
    int8_t lower = Config::GetPanelCalibration(LightPanelID::e_Lower, m_Value);

    printf_P(PSTR("Lower "));
    PrintPercentage(lower);
    printf_P(PSTR("/Upper "));
    PrintPercentage(upper);

    g_Display.PrintHorizontalBar(2, upper, 0);
    g_Display.PrintHorizontalBar(3, lower, 1);
}

bool MenuCalibration::Run(const Event& event)
{
    if ( event.type == EventID::e_KeyDown )
    {
        switch(event.value)
        {
            case KbdKeyCodes::e_Back:
                GlobalState::SetPrevState();
                return true;
            case KbdKeyCodes::e_Menu:
            case KbdKeyCodes::e_KnobButton:
                Config::SetPanelCalibration(m_Value);
                GlobalState::SetPrevState();
                return true;
        }
    }
    else if ( event.type == EventID::e_KnobTurned )
    {
        m_Value = clamp(-100, 100, m_Value + event.value);
        Print();
    }        
    return false;
}
