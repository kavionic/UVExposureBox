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
#include <stdlib.h>

#include "MenuExposure.h"
#include "GlobalState.h"
#include "PWMController.h"
#include "PanelController.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"
#include "Misc/Clock.h"

void MenuExposure::Initialize()
{
    m_TimeToShowCompressor = 0;
    m_FlashCycle           = 0;
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("Radiating"));
//    PrintCompressorOrProgress();
}

void MenuExposure::PrintPanelStats(int8_t displayLine, const char* name, int16_t time, int8_t power)
{
    g_Display.SetCursor(0,displayLine);
    char powerStr[PRINT_PERCENTAGE_BUFFER_SIZE];
    PrintPercentage(powerStr, power);
    if ( time >= 0 )
    {
        char timeStr[6];
        if ( time >= 0 && power != 0) {
            sprintf_P(timeStr, PSTR("%02u:%02u"), time / 60, time % 60);
        } else {
            strcpy_P(timeStr, PSTR("--:--"));
        }
        printf_P(PSTR("%S: %s - %s"), name, powerStr, timeStr);
    }
    else
    {
        printf_P(PSTR("%S: %s"), name, powerStr);
    }
    g_Display.ClearToEndOfLine();
}

void MenuExposure::PrintCompressorOrProgress(int8_t barDisplayLine)
{
//    if ( !m_TimeToShowCompressor && PanelController::GetState() == PanelState::e_Radiating )
//    {
        int16_t deltaTimeL;
        int16_t deltaTimeU;
        int16_t msTime;
        int16_t totalTime;
        bool    showCompressor = true;
        if ( PanelController::GetRadiationTimeLeft(&deltaTimeL, &deltaTimeU, &msTime, &totalTime) )
        {
            int16_t maxTime = max(deltaTimeL, deltaTimeU);
            g_Display.SetCursor(15,0);
            printf_P(PSTR("%02u:%02u"), maxTime / 60, maxTime % 60);
         
            if ( !m_TimeToShowCompressor && barDisplayLine != -1 )
            {
                int8_t progress = 100 - (I32(max(deltaTimeL, deltaTimeU)) * 100 - msTime / 10) / totalTime;
                g_Display.PrintHorizontalBar(barDisplayLine, progress);
                if ( barDisplayLine == 3 ) {
                    showCompressor = false;
                }
            }                
        }
        else
        {
            g_Display.SetCursor(15,0);
            g_Display.ClearToEndOfLine();
        }
//    }
    if ( showCompressor )
    {
        uint8_t speed = GlobalState::GetCompressorSpeed();
        char speedStr[4];
        if ( speed == 0 ) {
            sprintf_P(speedStr, PSTR("OFF"));
        } else if ( speed >= 100 ) {
            sprintf_P(speedStr, PSTR("MAX"));
        } else {
            sprintf_P(speedStr, PSTR("%2u%%"), speed);
        }            
        g_Display.SetCursor(0,3);
        printf_P(PSTR("Compressor: %u (%s)"), GlobalState::GetCompressorStep(), speedStr);
        g_Display.ClearToEndOfLine();
    }        
}

void MenuExposure::OnTimerTick100()
{
    PanelState::Enum state = PanelController::GetState();
    
    if ( m_TimeToShowCompressor ) m_TimeToShowCompressor--;
    
    if ( state == PanelState::e_Radiating )
    {
        PrintCompressorOrProgress(3);
    }
    else if ( state == PanelState::e_Paused )
    {
        if ( m_FlashCycle == 0 )
        {
            PrintCompressorOrProgress(1);
            g_Display.SetCursor(7,2);
            printf_P(PSTR("PAUSED"));
        }
        else if ( m_FlashCycle == 5 )
        {
            g_Display.SetCursor(15,0);
            g_Display.ClearToEndOfLine();
            g_Display.SetCursor(7,2);
            g_Display.ClearToEndOfLine();
        }
        if ( m_FlashCycle == 10 ) {
            m_FlashCycle = 0;
        } else {
            m_FlashCycle++;
        }                
    }
}

bool MenuExposure::Run(const Event& event)
{
    if ( event.type == EventID::e_RadiationUpdate )
    {
        PanelState::Enum state = PanelController::GetState();
        if ( event.value ) // State change
        {
            g_Display.ClearDisplay();
            g_Display.SetCursor(0, 0);
            printf_P(PSTR("Radiating"));
            m_FlashCycle = 0;
//            PrintCompressorOrProgress();
        }
        if ( state == PanelState::e_WaitingToStart )
        {
            g_Display.SetCursor(0,1);
            printf_P(PSTR("Starting in %uS"), PanelController::GetRemainingStartupDelay());
            g_Display.ClearToEndOfLine();
        }
        else if ( state == PanelState::e_Radiating )
        {
            int16_t deltaTimeL;
            int16_t deltaTimeU;

            if ( PanelController::GetRadiationTimeLeft(&deltaTimeL, &deltaTimeU) )
            {
                int16_t totalTime = max(deltaTimeL, deltaTimeU);
                g_Display.SetCursor(15,0);
                printf_P(PSTR("%02u:%02u"), totalTime / 60, totalTime % 60);
                
                PrintPanelStats(1, PSTR("Upper"), (deltaTimeU != deltaTimeL) ? deltaTimeU : -1, PanelController::GetRadiationPower(LightPanelID::e_Upper));
                PrintPanelStats(2, PSTR("Lower"), (deltaTimeU != deltaTimeL) ? deltaTimeL : -1, PanelController::GetRadiationPower(LightPanelID::e_Lower));
            }
        }
        else if ( state == PanelState::e_Paused )
        {
//            g_Display.SetCursor(7,1);
//            printf_P(PSTR("PAUSED"));
        }            
        else if ( state == PanelState::e_WaitingToStopCompressor )
        {
            g_Display.SetCursor(0,1);
            printf_P(PSTR("Stop compressor in:"), PanelController::GetRemainingCompressorStopDelay());
            g_Display.SetCursor(7,2);
            printf_P(PSTR("%u seconds"), PanelController::GetRemainingCompressorStopDelay());
            g_Display.ClearToEndOfLine();
        }
        else if ( state == PanelState::e_Idle )
        {
            GlobalState::SetPrevState();
        }
        return false;
    }
    else if ( event.type == EventID::e_TimeTick100 )
    {
        OnTimerTick100();
        return false;
    }        
    else if ( event.type == EventID::e_CompressorUpdate )
    {
        m_TimeToShowCompressor = 30;
        PrintCompressorOrProgress(-1);
        return false;
    }
    else if ( event.type == EventID::e_KeyDown )
    {
        switch( event.value )
        {
            case KbdKeyCodes::e_Back:
                GlobalState::SetPrevState();
                return true;
        }
    }
    else if ( event.type == EventID::e_KnobTurned )
    {
        GlobalState::SetCompressorSpeed(GlobalState::GetCompressorSpeed() + event.value);
        return true;
    }
    return false;
}
