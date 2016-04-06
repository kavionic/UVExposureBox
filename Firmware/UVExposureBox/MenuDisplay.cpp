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

#include "MenuDisplay.h"
#include "GlobalState.h"
#include "Config.h"
#include "PWMController.h"
#include "Keyboard.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"

enum
{
    e_MenuBacklight,
    e_MenuContrast,
    e_MenuBack    
};

static const char g_DisplayMenu0[] PROGMEM = "Backlight";
static const char g_DisplayMenu1[] PROGMEM = "Contrast";
static const char g_DisplayMenu2[] PROGMEM = "Back";

static const char* const g_DisplayMenuEntries[] PROGMEM = { g_DisplayMenu0, g_DisplayMenu1, g_DisplayMenu2};

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

static int8_t PrintDisplayMenuValue(void* userData, int8_t item, bool isSelected)
{
    if ( item == e_MenuBacklight || item == e_MenuContrast )
    {
        return printf_P((isSelected) ? PSTR(" [%d]%%") : PSTR(" %d%%"), (item == e_MenuBacklight) ? PWMController::GetLCDBacklight() : PWMController::GetLCDContrast());
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuDisplay::Initialize()
{
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("Display"));
    m_Menu.SetMenu(g_DisplayMenuEntries, ARRAY_COUNT(g_DisplayMenuEntries), PrintDisplayMenuValue, this, 1, 3);    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuDisplay::Run(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_MenuSelected:
            switch(event.value)
            {
                case e_MenuBacklight:
                    if ( !m_Menu.IsEditing() ) {
                        m_Menu.EditItem(true, PWMController::GetLCDBacklight());
                    } else {
                        m_Menu.EditItem(false, -1);
                        Config::SetLCDBacklight(PWMController::GetLCDBacklight());
                    }
                    break;
                case e_MenuContrast:
                    if ( !m_Menu.IsEditing() ) {
                        m_Menu.EditItem(true, PWMController::GetLCDContrast());
                    } else {
                        m_Menu.EditItem(false, -1);
                        Config::SetLCDContrast(PWMController::GetLCDContrast());
                    }
                    break;
                case e_MenuBack:
                case -1: // Cancel
//                    PWMController::SetLCDBacklight(eeprom_read_byte(&g_EEPROM.display.m_LCDBacklight));
//                    PWMController::SetLCDContrast(eeprom_read_byte(&g_EEPROM.display.m_LCDContrast));
                    GlobalState::SetPrevState();
                    break;
            }
            return true;
        case EventID::e_MenuItemEdited:
            switch(m_Menu.GetSelectedItem())
            {
                case e_MenuBacklight:
                {
                    int8_t value = clamp<int8_t>(0, 100, PWMController::GetLCDBacklight() + event.value);
//                    if ( value < 0 ) value = 0; else if ( value > 100 ) value = 100;
                    PWMController::SetLCDBacklight(value);
                    m_Menu.UpdateItem(m_Menu.GetSelectedItem(), value);
                    break;
                }                    
                case e_MenuContrast:
                {
                    int8_t value = clamp<int8_t>(0, 100, PWMController::GetLCDContrast() + event.value);
//                    if ( value < 0 ) value = 0; else if ( value > 100 ) value = 100;
                    PWMController::SetLCDContrast(value);
                    m_Menu.UpdateItem(m_Menu.GetSelectedItem(), value);
                    break;
                }
            }
            return true;
        default:
            return m_Menu.HandleEvent(event);
    }
}
