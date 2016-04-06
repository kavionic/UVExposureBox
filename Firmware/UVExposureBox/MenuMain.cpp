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

#include "MenuMain.h"
#include "Hardware.h"
#include "GlobalState.h"
#include "PWMController.h"
#include "Keyboard.h"
#include "ScrollMenu.h"


#include "Misc/Display.h"
#include "Misc/Utils.h"

int8_t MenuMain::s_Selection;

enum
{
    e_MenuDisplay,
    e_MenuSound,
    e_MenuCompressor,
    e_MenuCalibration,
    e_MenuDebug,
    e_MenuAbout,
    e_MenuBack
};

static const char g_SystemMenu0[] PROGMEM = "Display";
static const char g_SystemMenu1[] PROGMEM = "Sound";
static const char g_SystemMenu2[] PROGMEM = "Vacuum speed steps";
static const char g_SystemMenu3[] PROGMEM = "Calibration";
static const char g_SystemMenu4[] PROGMEM = "Debug";
static const char g_SystemMenu5[] PROGMEM = "About";
static const char g_SystemMenu6[] PROGMEM = "Back";

static const char* const g_SystemMenuEntries[] PROGMEM = {g_SystemMenu0, g_SystemMenu1, g_SystemMenu2, g_SystemMenu3, g_SystemMenu4, g_SystemMenu5, g_SystemMenu6};

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuMain::Initialize()
{
//    g_Display.SetCursor(0, 0);
//    printf_P(PSTR("System"));
        
    m_Menu.SetMenu(g_SystemMenuEntries, ARRAY_COUNT(g_SystemMenuEntries), nullptr, nullptr, 0, 3);
    m_Menu.SelectItem(s_Selection);
}        

bool MenuMain::Run(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_MenuSelected:
            if ( event.value != -1 ) s_Selection = event.value;
            switch(event.value)
            {
                case e_MenuDisplay:
                    GlobalState::SetState(MainState::e_DisplayMenu);
                    break;
                case e_MenuSound:
                    GlobalState::SetState(MainState::e_SoundMenu);
                    break;
                case e_MenuCompressor:
                    GlobalState::SetState(MainState::e_CompressorMenu);
                    break;
                case e_MenuCalibration:
                    GlobalState::SetState(MainState::e_CalibrationMenu);
                    break;
                case e_MenuAbout:
                    GlobalState::SetState(MainState::e_About);
                    break;
#ifdef ENABLE_DEBUG_MENU
                case e_MenuDebug:
                    GlobalState::SetState(MainState::e_DebugMenu);
                    break;
#endif // ENABLE_DEBUG_MENU
                //                        case 2: GlobalState::SetState(MainState::e_DisplayMenu); return;
                case e_MenuBack:
                case -1:
                    GlobalState::SetPrevState();
                    break;
            }
            return true;
        default:
            return m_Menu.HandleEvent(event);
            break;
    }
    return false;
}


