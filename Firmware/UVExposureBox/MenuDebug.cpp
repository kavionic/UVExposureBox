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

#include "MenuDebug.h"
#include "Hardware.h"
#include "GlobalState.h"
#include "Keyboard.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"
#include "Misc/DigitalPort.h"
#include "Misc/Clock.h"

#ifdef ENABLE_DEBUG_MENU

void MenuDebug::Initialize()
{
    m_BackKeyDownTime = 0;
    m_FirstCharacter = 0;
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("Debug"));    
}

bool MenuDebug::Run(const Event& event)
{
    char keyStr[16];
    for ( uint8_t i = 0 ; i < 16 ; ++i )
    {
        keyStr[i] = ( Keyboard::GetKeyState(i) ) ? '*' : '.';
    }
    g_Display.SetCursor(11, 0);
    printf_P(PSTR("%d  "), Keyboard::GetKnobPos());
    
    uint8 portVal = DigitalPort::Get(PORT_ROT);
    uint8_t  encoderValue = ((portVal & PIN_ROT_TRIG_bm) >> PIN_ROT_TRIG_bp) | ((portVal & PIN_ROT_DIR_bm) >> (PIN_ROT_DIR_bp) << 1);
//    uint8_t encoderValue = ((portVal & PIN_ROT_TRIG) ? 1 : 0) + ((portVal & PIN_ROT_DIR) ? 2 : 0);    
    g_Display.SetCursor(15, 0);
    printf_P(PSTR("%d"), encoderValue);
    
    g_Display.SetCursor(0,1);
    printf_P(PSTR("%.16s"), keyStr);
    
    g_Display.SetCursor(0,2);
    printf_P(PSTR("%d  "), m_FirstCharacter);
    g_Display.SetCursor(4,2);
    for ( uint8_t i = 0 ; i < 16 ; ++i )
    {
        g_Display.WriteData(m_FirstCharacter + i);
    }
    
    g_Display.SetCursor(0,3);
    printf_P(PSTR("%02u:%02u:%02u:%02u (%u)"), Clock::GetHour(), Clock::GetMinutes(), Clock::GetSeconds(), Clock::GetHundreds(), GlobalState::s_KeyScanTime);
    
    if ( m_BackKeyDownTime != 0 && (Clock::GetTime() - m_BackKeyDownTime) > 2000 )
    {
        GlobalState::SetPrevState();
    }
    switch(event.type)
    {
        case EventID::e_KeyDown:
        {
            if ( event.value == KbdKeyCodes::e_Back ) m_BackKeyDownTime = Clock::GetTime();
            const char* keyName = Keyboard::GetKeyNameP(event.value);
            if ( keyName != NULL )
            {
                g_Display.SetCursor(6,0);
                printf_P(keyName);
            }
            return true;
        }
        case EventID::e_KeyUp:
            if ( event.value == KbdKeyCodes::e_Back ) m_BackKeyDownTime = 0;
            return true;
        case EventID::e_KnobTurned:
            m_FirstCharacter += event.value;
            return true;
        default:
            return false;
    }
}

#endif // ENABLE_DEBUG_MENU
