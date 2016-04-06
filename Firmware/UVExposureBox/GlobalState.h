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


#ifndef __GLOBALSTATE_H__
#define __GLOBALSTATE_H__

#include "Hardware.h"
#include "RuntimeTracker.h"
#include "Keyboard.h"

#define ENABLE_DEBUG_MENU

namespace MainState
{
    enum Enum
    {
        e_None,
        e_Home,
//        e_PanelUMenu,
//        e_PanelLMenu,
        e_CompressorMenu,
        e_MainMenu,
        e_DisplayMenu,
        e_SoundMenu,
        e_CalibrationMenu,
        //        e_DisplayContrast,
        e_Exposure,
        e_About,
#ifdef ENABLE_DEBUG_MENU
        e_DebugMenu,
#endif
    };
}

namespace EventID
{
    enum Enum
    {
        e_None,
        e_TimeTick100,
        e_TimeTick500,
        e_TimeTick1000,
        e_KeyDown,
        e_KeyRepeat,
        e_KeyUp,
        e_KnobTurned,
        e_MenuSelected,
        e_MenuItemEdited,
        e_ValueChanged,
        e_ValueSelected,
        e_TextEditCanceled,
        e_TextEditDone,
        e_PresetSelected,
        e_RadiationUpdate,
        e_CompressorUpdate
    };
}

struct Event
{
    Event() {}
    Event(EventID::Enum t, int8_t v) : type(t), value(v) {}
    
    EventID::Enum type;
    int8_t        value;
};

class GlobalState
{
public:
    static volatile uint16_t       s_KeyScanTime;
    
    static void Initialize();
    static void Run();

    static RuntimeTracker& GetRuntimeTracker() { return s_RuntimeTracker; }

    static void CompressorDown();
    static void CompressorUp();
    static void StopCompressor();
    
    static uint8_t GetCompressorStep();
    static void    SetCompressorSpeed(int8_t speed);
    static int8_t  GetCompressorSpeed(uint8_t speed);
    static int8_t  GetCompressorSpeed();
        
    static void SetState(MainState::Enum state, bool pushToStack = true);
    static void SetPrevState();
    static MainState::Enum GetState() { return s_State; }    

    static void AddEvent(const Event& event);
    static bool GetEvent(Event* event);
    static Event* PeekEvent();
    
private:
    static void CallShutdown(MainState::Enum state);
    static void CallInitialize(MainState::Enum state);
    static bool CallRun(MainState::Enum state, const Event& event);
    
    static const uint8_t STATE_STACK_SIZE = 4;
    static MainState::Enum s_State;
    static MainState::Enum s_PrevStates[STATE_STACK_SIZE];
    static bool            s_StandBy;
    static uint8_t         s_CompressorStep;

    static RuntimeTracker  s_RuntimeTracker;

    static EventQueue<Event, 8> s_EventQueue;
};

#endif //__GLOBALSTATE_H__
