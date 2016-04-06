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
#include <string.h>

#include "Misc/Display.h"
#include "Misc/Clock.h"
#include "Misc/Utils.h"

#include "MenuCompressorSteps.h"
#include "GlobalState.h"
#include "Config.h"
#include "PWMController.h"
#include "Keyboard.h"

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuCompressorSteps::Initialize()
{
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("Vacuum speed steps"));

    eeprom_read_block(m_Speeds, g_EEPROM.global.m_CompressorSpeeds, sizeof(m_Speeds));
    
    for ( m_StepCount = 0 ; m_StepCount < COMPRESSOR_SPEED_STEPS && m_Speeds[m_StepCount] != 0 ; ++m_StepCount );
    m_StepCount += 2; // Added the implicit 0% and 100% values at each end.
    
    m_EditMode      = false;
    m_RunCompressor = false;
    m_Selection = 0;    
    m_FlashCount = 0;
    Print();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t MenuCompressorSteps::GetValue(uint8_t index) const
{
    if ( index == 0 ) {
        return 0;
    } else if ( index >= m_StepCount - 1) {
        return 100;
    } else {
        return m_Speeds[index-1];
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuCompressorSteps::PrintLine(uint8_t line)
{
    g_Display.SetCursor(0, 1 + line);

    int8_t start = line * 6;
    int8_t end = min(int8_t(start + 6), m_StepCount);

    int8_t charCount = 0;
    for ( int8_t i = start ; i < end ; ++i )
    {
        uint8_t value = GetValue(i);
        char valueStr[4];
        if ( (m_FlashCount & 0x01) && i == m_Selection ) {
            sprintf_P(valueStr, (value == 100) ? PSTR("   ") : PSTR("  ") );            
        } else if ( value == 100 ) {
            sprintf_P(valueStr, PSTR("MAX") );
        } else {
            sprintf_P(valueStr, PSTR("%02u"), value);
        }            
        charCount += printf_P((i!=m_Selection) ? PSTR("%s ") : ((m_EditMode) ? PSTR("[%s] ") : PSTR("<%s> ")), valueStr);
    }
    for (; charCount < DisplayLCD::WIDTH ; ++charCount)
    {
        g_Display.WriteData(' ');
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuCompressorSteps::Print()
{
    PrintLine(0);
    PrintLine(1);
    PrintLine(2);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuCompressorSteps::Flash()
{
    m_FlashCount = 3;
    m_NextFlashTime = Clock::GetTime() + 100;
    Print();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuCompressorSteps::AddStep()
{
    if ( m_StepCount < COMPRESSOR_SPEED_STEPS + 2 )
    {
        if ( m_Selection == m_StepCount - 1 ) {
            if (GetValue(m_Selection - 1) == 99 ) return false; // No room between last step and the previous.
            m_Selection--; // Special case when last step selected. Add new entry before rather than after selection.
        } else if ( GetValue(m_Selection) == GetValue(m_Selection+1) - 1 ) {
            return false; // No room between selection and next step.
        }

        if ( m_Selection < m_StepCount - 2 ) {
            memmove(&m_Speeds[m_Selection + 1], &m_Speeds[m_Selection], m_StepCount - m_Selection - 2);
        }
        m_Selection++;
        m_StepCount++;
        
        if ( (m_StepCount - 2) < COMPRESSOR_SPEED_STEPS ) m_Speeds[m_StepCount-2] = 0;
        m_Speeds[m_Selection -1] = (GetValue(m_Selection-1) + GetValue(m_Selection + 1)) >> 1;
        Print();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuCompressorSteps::DeleteStep()
{
    if ( m_StepCount > 2 && m_Selection != 0 && m_Selection != m_StepCount - 1 )
    {
        memmove(&m_Speeds[m_Selection - 1], &m_Speeds[m_Selection], m_StepCount - m_Selection - 2 );
        m_StepCount--;
        m_Speeds[m_StepCount-2] = 0;

        if ( m_Selection >= m_StepCount ) m_Selection = m_StepCount - 1;
        Print();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuCompressorSteps::Run(const Event& event)
{
    if ( m_FlashCount > 0 )
    {
        uint32_t curTime = Clock::GetTime();
        if ( curTime > m_NextFlashTime )
        {
            m_FlashCount--;
            if ( m_FlashCount != 0 )
            {
                m_NextFlashTime = curTime + 100;
            }
            Print();
        }
        return true;
    }

    if ( event.type == EventID::e_KeyDown )
    {
        switch(event.value)
        {
            case KbdKeyCodes::e_KnobButton:
                m_EditMode = !m_EditMode;
                Print();
                return true;
            case KbdKeyCodes::e_ComprDown:
                if ( !DeleteStep() ) Flash();
                return true;
            case KbdKeyCodes::e_ComprUp:
                if ( !AddStep() ) Flash();
                return true;
            case KbdKeyCodes::e_Start:
                m_RunCompressor = true;
                PWMController::SetCompressor(GetValue(m_Selection));
                return true;
            case KbdKeyCodes::e_Stop:
                m_RunCompressor = false;
                PWMController::SetCompressor(0);
                return true;
            case KbdKeyCodes::e_Back:
                if ( m_RunCompressor ) {
                    PWMController::SetCompressor(0);
                    m_RunCompressor = false;
                }
                eeprom_update_block(m_Speeds, g_EEPROM.global.m_CompressorSpeeds, min(COMPRESSOR_SPEED_STEPS, int8_t(m_StepCount - 1)));
                GlobalState::SetPrevState();
                return true;
        }
    }
    else if ( event.type == EventID::e_KnobTurned )
    {
        if ( m_EditMode )
        {
            if ( m_Selection == 0 )
            {
                if ( event.value <= 0 || !AddStep() ) Flash();
            }
            else if ( m_Selection == (m_StepCount - 1) )
            {
                if ( event.value >= 0 || !AddStep() ) Flash();
            }
            else if ( m_StepCount > 2 )
            {
                int8_t delta = event.value;
                while(delta) // Avoid the complexity of potentially stepping over multiple neighbors.
                {
                    if (delta < 0) {
                        m_Speeds[m_Selection - 1]--;
                        delta++;
                    } else {
                        m_Speeds[m_Selection - 1]++;                    
                        delta--;
                    }
                    for (;;)
                    {
                        // Bubble the new value into order.
                        if ( m_Selection > 1 && m_Speeds[m_Selection-1] == m_Speeds[m_Selection-2] )
                        {
                            swap(m_Speeds[m_Selection-1], m_Speeds[m_Selection-2]);
                            m_Selection--;
                            m_Speeds[m_Selection-1]--;
                        }
                        else if ( m_Selection < m_StepCount - 2 && m_Speeds[m_Selection-1] == m_Speeds[m_Selection] )
                        {
                            swap(m_Speeds[m_Selection-1], m_Speeds[m_Selection]);
                            m_Selection++;
                            m_Speeds[m_Selection-1]++;
                        }
                        else
                        {
                            if ( m_Speeds[m_Selection - 1] <= 0 )
                            {
                                DeleteStep();
                                m_Selection = 0;
                            }
                            else if ( m_Speeds[m_Selection-1] > 99 )
                            {
                                DeleteStep();
                            }
                            break;
                        }
                    }                        
                }
            }
        }            
        else
        {
            int8_t delta = event.value;
            while(delta) // Avoid the complexity of potentially stepping over multiple neighbors.
            {
                if ( delta < 0 )
                {
                    m_Selection = (m_Selection > 0) ? m_Selection - 1 : m_StepCount - 1;
                    delta++;
                }
                else
                {                        
                    m_Selection = (m_Selection < (m_StepCount-1)) ? m_Selection + 1 : 0;
                    delta--;
                }
            }                
        }
        if ( m_RunCompressor ) {
            PWMController::SetCompressor(GetValue(m_Selection));
        }            
        Print();
        return true;
    }
    return false;
}
