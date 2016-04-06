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


#include "PanelController.h"
#include "GlobalState.h"
#include "Config.h"
#include "PWMController.h"
#include "Beeper.h"

PanelState::Enum PanelController::s_State;
uint16_t         PanelController::s_RadiationTimeMS;
int8_t           PanelController::s_StartDelay;
int8_t           PanelController::s_CompressorStopDelay;
int16_t          PanelController::s_TotalTime;
int16_t          PanelController::s_PanelTimeLeft[LightPanelID::e_Count];
uint8_t          PanelController::s_PanelPower[LightPanelID::e_Count];

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PanelController::StartRadiation()
{
    bool isIdle;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        isIdle = s_State == PanelState::e_Idle;
    }        
    if ( isIdle )
    {
        s_StartDelay = 3;
        s_CompressorStopDelay = Config::GetPresetCompressorStopDelay();
        s_PanelPower[LightPanelID::e_Lower] = Config::GetPresetPanelPower(LightPanelID::e_Lower);
        s_PanelPower[LightPanelID::e_Upper] = Config::GetPresetPanelPower(LightPanelID::e_Upper);
        s_PanelTimeLeft[LightPanelID::e_Lower] = Config::GetPresetPanelDuration(LightPanelID::e_Lower);
        s_PanelTimeLeft[LightPanelID::e_Upper] = Config::GetPresetPanelDuration(LightPanelID::e_Upper);
        s_TotalTime = max(s_PanelTimeLeft[LightPanelID::e_Lower], s_PanelTimeLeft[LightPanelID::e_Upper]);
        
        s_RadiationTimeMS = 1000;
        
        if ( s_StartDelay > 0 )
        {
            s_State = PanelState::e_WaitingToStart;
                
        }
        else
        {
            s_State = PanelState::e_Radiating;
            for ( int8_t i = 0 ; i < LightPanelID::e_Count ; ++i ) {
                if ( s_PanelTimeLeft[i] ) PWMController::SetLightPanel(LightPanelID::Enum(i), s_PanelPower[i]);
            }                
        }            
        GlobalState::AddEvent(Event(EventID::e_RadiationUpdate, true));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PanelController::SkipStartupDelay()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if ( s_State == PanelState::e_WaitingToStart )
        {
            s_StartDelay = 1;
            s_RadiationTimeMS = 1;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PanelController::SkipCompressorStop(bool stopCompressor)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if ( s_State == PanelState::e_WaitingToStopCompressor )
        {
            s_State = PanelState::e_Idle;
            s_CompressorStopDelay = 0;
            if ( stopCompressor )
            {
                GlobalState::StopCompressor();
            }
            GlobalState::AddEvent(Event(EventID::e_RadiationUpdate, true));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PanelController::PauseRadiation(bool pause)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if ( pause )
        {
            if ( s_State == PanelState::e_Radiating )
            {
                for ( int8_t i = 0 ; i < LightPanelID::e_Count ; ++i ) {
                    if ( s_PanelTimeLeft[i] ) PWMController::SetLightPanel(LightPanelID::Enum(i), 0);
                }
                s_State = PanelState::e_Paused;
                GlobalState::AddEvent(Event(EventID::e_RadiationUpdate, true));
            }
        }
        else
        {
            if ( s_State == PanelState::e_Paused )
            {
                s_State = PanelState::e_Radiating;
                GlobalState::AddEvent(Event(EventID::e_RadiationUpdate, true));
                for ( int8_t i = 0 ; i < LightPanelID::e_Count ; ++i ) {
                    if ( s_PanelTimeLeft[i] ) PWMController::SetLightPanel(LightPanelID::Enum(i), s_PanelPower[i]);
                }
            }
        }            
    }                    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

/*bool PanelController::IsPaused()
{
    return s_IsRadiationPaused;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PanelController::StopRadiation()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if ( s_State == PanelState::e_WaitingToStart ) {
            s_State = PanelState::e_Idle;
            GlobalState::AddEvent(Event(EventID::e_RadiationUpdate, true));
        } else if ( s_State == PanelState::e_Radiating || s_State == PanelState::e_Paused ) {
            s_PanelTimeLeft[LightPanelID::e_Lower] = 1;
            s_PanelTimeLeft[LightPanelID::e_Upper] = 1;
            s_RadiationTimeMS = 1;
            if ( s_State == PanelState::e_Paused )
            {
                PauseRadiation(false);
            }
        }/* else if ( s_State == PanelState::e_WaitingToStopCompressor ) {            
            s_CompressorStopDelay = 1;
            s_RadiationTimeMS = 1;
        }*/            
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool PanelController::GetRadiationTimeLeft(int16_t* lowerPanel, int16_t* upperPanel, int16_t* msTime, int16_t* totalTime)
{
    bool isRadiating = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if ( s_State != PanelState::e_Idle && s_State != PanelState::e_WaitingToStopCompressor )
        {
            isRadiating = true;
            *lowerPanel = s_PanelTimeLeft[LightPanelID::e_Lower];
            *upperPanel = s_PanelTimeLeft[LightPanelID::e_Upper];
            if ( totalTime != nullptr ) *totalTime = s_TotalTime;
            if ( msTime != nullptr )    *msTime = 1000 - s_RadiationTimeMS;
        }
        else
        {
            *lowerPanel = 0;
            *upperPanel = 0;
        }
    }
    return isRadiating;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t PanelController::GetRadiationPower(LightPanelID::Enum panelID)
{
    int8_t power = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        power = (s_State == PanelState::e_Radiating && s_PanelTimeLeft[panelID] > 0) ? s_PanelPower[panelID] : 0;
    }        
    return power;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PanelController::UpdateRadiation()
{
    if ( s_State != PanelState::e_Idle && s_State != PanelState::e_Paused )
    {
        s_RadiationTimeMS--;        
        if ( !s_RadiationTimeMS )
        {
            s_RadiationTimeMS = 1000;
            PanelState::Enum prevState = s_State;
            if ( s_State == PanelState::e_Radiating )
            {
                int8_t activePanels = 0;
                
                bool doBeep = false;
                for ( int8_t i = 0 ; i < LightPanelID::e_Count ; ++i )
                {
                    LightPanelID::Enum panelID = LightPanelID::Enum(i);
                    if ( s_PanelTimeLeft[i] )
                    {
                        activePanels++;
                        s_PanelTimeLeft[i]--;
                        if ( !s_PanelTimeLeft[i] ) {
                            activePanels--;
                            PWMController::SetLightPanel(panelID, 0);
                        } else if ( s_PanelTimeLeft[i] <= 3 ) {
                            doBeep = true;
                        }
                    }
                }
                if ( !activePanels )
                {
                    Beeper::Alarm();
                    if ( s_CompressorStopDelay < 0 || PWMController::GetCompressor() == 0 ) {
                        s_State = PanelState::e_Idle;
                    } else if ( s_CompressorStopDelay == 0 ) {
                        GlobalState::StopCompressor();
                        s_State = PanelState::e_Idle;
                    } else {
                        s_State = PanelState::e_WaitingToStopCompressor;                        
                    }
                }
                else if ( doBeep )
                {
                    Beeper::Beep(BeepID::e_FinishCountDown);                    
                }
            }
            else if ( s_State == PanelState::e_WaitingToStart )
            {
                if ( !(--s_StartDelay) ) {
                    for ( int8_t i = 0 ; i < LightPanelID::e_Count ; ++i ) {
                        if ( s_PanelTimeLeft[i] ) PWMController::SetLightPanel(LightPanelID::Enum(i), s_PanelPower[i]);
                    }
                    s_State = PanelState::e_Radiating;
                    Beeper::Beep(BeepID::e_StartRadiating);
                } else if ( s_StartDelay < 3 ) {
                    Beeper::Beep(BeepID::e_StartCountDown);
                }
            }
            else if ( s_State == PanelState::e_WaitingToStopCompressor )
            {
                if ( !(--s_CompressorStopDelay) ) {
                    GlobalState::StopCompressor();
                    s_State = PanelState::e_Idle;
                }
            }
            GlobalState::AddEvent(Event(EventID::e_RadiationUpdate, s_State != prevState));
        }
    }                        
}
