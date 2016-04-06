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

#include <string.h>
#include <stdlib.h>

#include "GlobalState.h"
#include "Keyboard.h"
#include "PWMController.h"
#include "PanelController.h"
#include "Config.h"

#include "MenuHome.h"
#include "MenuExposure.h"
#include "MenuAbout.h"
#include "MenuCompressorSteps.h"
#include "MenuMain.h"
#include "MenuDisplay.h"
#include "MenuSound.h"
#include "MenuCalibration.h"
#include "MenuDebug.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"

MainState::Enum GlobalState::s_State = MainState::e_Home;
MainState::Enum GlobalState::s_PrevStates[STATE_STACK_SIZE];
bool            GlobalState::s_StandBy;

uint8_t         GlobalState::s_CompressorStep;
RuntimeTracker  GlobalState::s_RuntimeTracker;

volatile uint16_t GlobalState::s_KeyScanTime;

EventQueue<Event, 8> GlobalState::s_EventQueue;

static union MenuUnion
{
    MenuHome            homeMenu;
    MenuExposure        exposureMenu;
    MenuMain            mainMenu;
    MenuCompressorSteps compressorMenu;
    MenuDisplay         displayMenu;
    MenuSound           soundMenu;
    MenuCalibration     calibrationMenu;
    MenuAbout           aboutMenu;
#ifdef ENABLE_DEBUG_MENU
    MenuDebug           debugMenu;
#endif // ENABLE_DEBUG_MENU
} g_MenuUnion;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::Initialize()
{
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::CallInitialize(MainState::Enum state)
{
    switch(state)
    {
        case MainState::e_None:
            break;
        case MainState::e_Exposure:
            g_MenuUnion.exposureMenu.Initialize();
            break;
        case MainState::e_Home:
            g_MenuUnion.homeMenu.Initialize();
            break;
        case MainState::e_MainMenu:
            g_MenuUnion.mainMenu.Initialize();
            break;
        case MainState::e_CompressorMenu:
            g_MenuUnion.compressorMenu.Initialize();
            break;
        case MainState::e_DisplayMenu:
            g_MenuUnion.displayMenu.Initialize();
            break;
        case MainState::e_SoundMenu:
            g_MenuUnion.soundMenu.Initialize();
            break;
        case MainState::e_CalibrationMenu:
            g_MenuUnion.calibrationMenu.Initialize();
            break;
        case MainState::e_About:
            g_MenuUnion.aboutMenu.Initialize();
            break;
#ifdef ENABLE_DEBUG_MENU
        case MainState::e_DebugMenu:
            g_MenuUnion.debugMenu.Initialize();
            break;
#endif // ENABLE_DEBUG_MENU
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::CallShutdown(MainState::Enum state)
{
    switch(state)
    {
        case MainState::e_None:
            break;
        case MainState::e_Exposure:
            g_MenuUnion.exposureMenu.Shutdown();
            break;
        case MainState::e_Home:
            g_MenuUnion.homeMenu.Shutdown();
            break;
        case MainState::e_MainMenu:
            g_MenuUnion.mainMenu.Shutdown();
            break;
        case MainState::e_CompressorMenu:
            g_MenuUnion.compressorMenu.Shutdown();
            break;
        case MainState::e_DisplayMenu:
            g_MenuUnion.displayMenu.Shutdown();
            break;
        case MainState::e_SoundMenu:
            g_MenuUnion.soundMenu.Shutdown();
            break;
        case MainState::e_CalibrationMenu:
            g_MenuUnion.calibrationMenu.Shutdown();
            break;
        case MainState::e_About:
            g_MenuUnion.aboutMenu.Shutdown();
            break;
#ifdef ENABLE_DEBUG_MENU
        case MainState::e_DebugMenu:
            g_MenuUnion.debugMenu.Shutdown();
            break;
#endif // ENABLE_DEBUG_MENU
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool GlobalState::CallRun(MainState::Enum state, const Event& event)
{
    switch(s_State)
    {
        case MainState::e_None:
            SetState(MainState::e_Home);
            return false;
        case MainState::e_Exposure:
            return g_MenuUnion.exposureMenu.Run(event);
        case MainState::e_Home:
            return g_MenuUnion.homeMenu.Run(event);
        case MainState::e_MainMenu:
            return g_MenuUnion.mainMenu.Run(event);
        case MainState::e_CompressorMenu:
            return g_MenuUnion.compressorMenu.Run(event);
        case MainState::e_DisplayMenu:
            return g_MenuUnion.displayMenu.Run(event);
        case MainState::e_SoundMenu:
            return g_MenuUnion.soundMenu.Run(event);
        case MainState::e_CalibrationMenu:
            return g_MenuUnion.calibrationMenu.Run(event);
        case MainState::e_About:
            return g_MenuUnion.aboutMenu.Run(event);
#ifdef ENABLE_DEBUG_MENU
        case MainState::e_DebugMenu:
            return g_MenuUnion.debugMenu.Run(event);
#endif // ENABLE_DEBUG_MENU
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::Run()
{
    s_RuntimeTracker.Restart();
    
    MainState::Enum prevState = MainState::e_None;
    while(1)
    {
        Event event;
                
        if ( !GetEvent(&event) )
        {
            event.type = EventID::e_None;
        }
        
        if ( s_StandBy )
        {
            if ( event.type == EventID::e_KeyDown && event.value == KbdKeyCodes::e_Power ) {
                s_StandBy = false;
                PWMController::SetLCDBacklight(Config::GetLCDBacklight());
                PWMController::SetLCDContrast(Config::GetLCDContrast());
                g_Display.EnableDisplay(true);
                s_RuntimeTracker.Restart();
            }
            continue;
        } else if ( event.type == EventID::e_KeyDown && event.value == KbdKeyCodes::e_Power ) {
            s_RuntimeTracker.Update(true);
            PWMController::SetLCDBacklight(0);
            PWMController::SetLCDContrast(0);
            g_Display.EnableDisplay(false);
            s_StandBy = true;
            SetState(MainState::e_Home);
            continue;
        }

        s_RuntimeTracker.Update(false);

        if ( event.type == EventID::e_KeyDown && PanelController::GetState() != PanelState::e_Idle )
        {
            switch( event.value )
            {
                case KbdKeyCodes::e_Start:
                    if ( s_State == MainState::e_Exposure )
                    {
                        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
                        {
                            PanelState::Enum state = PanelController::GetState();
                            if ( state == PanelState::e_WaitingToStart ) {
                                PanelController::SkipStartupDelay();
                            } else if ( state == PanelState::e_Radiating ) {
                                PanelController::PauseRadiation(true);
                            } else if ( state == PanelState::e_Paused ) {
                                PanelController::PauseRadiation(false);
                            } else if ( state == PanelState::e_WaitingToStopCompressor ) {
                                PanelController::SkipCompressorStop(false);
                            }
                        }
                    }                        
                    else
                    {
                        SetState(MainState::e_Exposure);
                    }
                    continue;
                case KbdKeyCodes::e_Stop:
                    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
                    {
                        if ( PanelController::GetState() == PanelState::e_WaitingToStopCompressor ) {
                            PanelController::SkipCompressorStop(true);
                        } else {
                            PanelController::StopRadiation();
                        }
                    }
                    continue;
            }
        }
        if ( s_State != prevState ) {
            if ( prevState != MainState::e_None ) {
                CallShutdown(prevState);
            }
            if ( s_State != MainState::e_None ) {
                CallInitialize(s_State);
            }
            prevState = s_State;
        }
        bool wasEventHandled = CallRun(s_State, event);

        if ( !wasEventHandled && event.type == EventID::e_KeyDown )
        {
            switch(event.value)
            {
                case KbdKeyCodes::e_ComprDown:
                    CompressorDown();
                    break;
                case KbdKeyCodes::e_ComprUp:
                    CompressorUp();
                    break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::SetState(MainState::Enum state, bool pushToStack)
{
    if ( state != s_State )
    {
        if ( pushToStack )
        {
            memmove(&s_PrevStates[1], &s_PrevStates[0], sizeof(s_PrevStates) - sizeof(s_PrevStates[0]));
            s_PrevStates[0] = s_State;
        }
        s_State = state;
        g_Display.ClearDisplay();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::SetPrevState()
{
    MainState::Enum newState = MainState::e_Home;
    
    if (s_PrevStates[0] != MainState::e_None )
    {
        newState = s_PrevStates[0];
        memmove(&s_PrevStates[0], &s_PrevStates[1], sizeof(s_PrevStates) - sizeof(s_PrevStates[0]));
        s_PrevStates[ARRAY_COUNT(s_PrevStates)-1] = MainState::e_None;
    }
    SetState(newState, false);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::CompressorDown()
{
    if ( s_CompressorStep != 0 )
    {
        s_CompressorStep--;
    }
    if ( GetCompressorSpeed() != GetCompressorSpeed(s_CompressorStep) ) {
        PWMController::SetCompressor(GetCompressorSpeed(s_CompressorStep));
        AddEvent(Event(EventID::e_CompressorUpdate, s_CompressorStep));        
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::CompressorUp()
{
    if ( s_CompressorStep < ARRAY_COUNT(g_EEPROM.global.m_CompressorSpeeds) && eeprom_read_byte(&g_EEPROM.global.m_CompressorSpeeds[s_CompressorStep-1]) != 0 )
    {
        s_CompressorStep++;
    }
    if ( GetCompressorSpeed() != GetCompressorSpeed(s_CompressorStep) ) {
        PWMController::SetCompressor(GetCompressorSpeed(s_CompressorStep));
        AddEvent(Event(EventID::e_CompressorUpdate, s_CompressorStep));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::StopCompressor()
{
    s_CompressorStep = 0;
    PWMController::SetCompressor(0);        
    AddEvent(Event(EventID::e_CompressorUpdate, s_CompressorStep));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t GlobalState::GetCompressorStep()
{
    return s_CompressorStep;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::SetCompressorSpeed(int8_t speed)
{
    speed = max(int8_t(0), min(int8_t(100), speed));
    PWMController::SetCompressor(speed);
    int8_t i = 0;
    int8_t smallestError = 127;
    int8_t stepSpeed;
    do 
    {
        stepSpeed = GetCompressorSpeed(i);
        int8_t error = abs(stepSpeed - speed);
        if ( error < smallestError ) {
             smallestError = error;
             s_CompressorStep = i;
        }
        ++i;
    } while (stepSpeed != 100);
    AddEvent(Event(EventID::e_CompressorUpdate, s_CompressorStep));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t GlobalState::GetCompressorSpeed()
{
    return PWMController::GetCompressor();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t GlobalState::GetCompressorSpeed(uint8_t step)
{
    if ( step == 0 ) {
        return 0;
    } else if ( step < ARRAY_COUNT(g_EEPROM.global.m_CompressorSpeeds) && eeprom_read_byte(&g_EEPROM.global.m_CompressorSpeeds[step-1]) != 0 ) {
        return eeprom_read_byte(&g_EEPROM.global.m_CompressorSpeeds[step-1]);
    } else {
        return 100;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GlobalState::AddEvent( const Event& event )
{
    s_EventQueue.AddEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool GlobalState::GetEvent( Event* event )
{
    return s_EventQueue.GetEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

Event* GlobalState::PeekEvent()
{
    return s_EventQueue.PeekEvent();
}
