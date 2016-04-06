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

#include "ValueEditor.h"
#include "Hardware.h"
#include "GlobalState.h"

#include "Misc/Display.h"

// default constructor
ValueEditor::ValueEditor()
{
}

void ValueEditor::Setup(const char* label, int16_t minValue, int16_t maxValue, int16_t startValue, ValuePrintCallback_t* printCallback)
{
    m_MinValue = minValue;
    m_MaxValue = maxValue;
    m_CurrentValue = startValue;
    
    m_PrintCallback = printCallback;
    
    g_Display.SetCursor(0, 1);
    m_LabelWidth = printf_P(label) + 1;
    
    Print();
}

void ValueEditor::HandleEvent(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_KeyDown:
            switch(event.value)
            {
                case KbdKeyCodes::e_KnobButton:
                    GlobalState::AddEvent(Event(EventID::e_ValueSelected, m_CurrentValue));
                    break;
                case KbdKeyCodes::e_Menu:
                    GlobalState::AddEvent(Event(EventID::e_ValueSelected, m_CurrentValue));
                    break;
            }
            break;
        case EventID::e_KnobTurned:
            m_CurrentValue += event.value;
            if ( m_CurrentValue < m_MinValue ) m_CurrentValue = m_MinValue;
            else if ( m_CurrentValue > m_MaxValue ) m_CurrentValue = m_MaxValue;
            Print();
            GlobalState::AddEvent(Event(EventID::e_ValueChanged, m_CurrentValue));
            break;
        default:
            break;
    }    
}

void ValueEditor::Print()
{
    g_Display.SetCursor(m_LabelWidth, 1);
    m_PrintCallback(m_CurrentValue);
    
    uint16_t percentage = m_CurrentValue * 20 * 5 / (m_MaxValue - m_MinValue);
    uint8_t fullChars = percentage / 5;
    uint8_t pixels = percentage % 5;

    uint8_t charBits = ((1<<pixels) - 1) << (5 - pixels);

    // Render custom character for partial block.    
    g_Display.WriteCustomCharBegin(0);
    for ( int8_t i = 0 ; i < 8 ; ++i )
    {
        g_Display.WriteCustomCharRow(charBits);        
    }
    
    // Print all full blocks.
    g_Display.SetCursor(0, 2);
    for ( int8_t i = 0 ; i < fullChars ; ++i )
    {
        g_Display.WriteData(255);
    }
    // Print the partial block.
    g_Display.WriteData(0);
    
    // Print all empty blocks.
    for ( int8_t i = fullChars + 1 ; i < 20 ; ++i )
    {
        g_Display.WriteData(' ');
    }
}

