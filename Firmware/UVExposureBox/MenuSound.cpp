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

#include "MenuSound.h"
#include "GlobalState.h"
#include "Hardware.h"
#include "Config.h"
#include "PWMController.h"
#include "Keyboard.h"
#include "Beeper.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"

enum
{
    e_MenuVolume,
    e_MenuFrequency,
    
    e_MenuFirstBeep,
    e_MenuKeyPress = e_MenuFirstBeep,
    e_MenuKeyRepeat,
    e_MenuLongPress,
    e_MenuKnob,
    e_MenuCancel,
    e_MenuStartCountDown,
    e_MenuFinishCountDown,
    e_MenuLastBeep,
    e_MenuStartRadiating = e_MenuLastBeep,
    e_MenuBack    
};

static const char g_SoundMenu0[] PROGMEM = "Volume";
static const char g_SoundMenu1[] PROGMEM = "Frequency";
static const char g_SoundMenu2[] PROGMEM = "KeyPress";
static const char g_SoundMenu3[] PROGMEM = "KeyRepeat";
static const char g_SoundMenu4[] PROGMEM = "LongPress";
static const char g_SoundMenu5[] PROGMEM = "Knob";
static const char g_SoundMenu6[] PROGMEM = "Cancel";
static const char g_SoundMenu7[] PROGMEM = "Start Cnt";
static const char g_SoundMenu8[] PROGMEM = "End Cnt";
static const char g_SoundMenu9[] PROGMEM = "Start";

static const char g_SoundMenu10[] PROGMEM = "Back";

static const char* const g_SoundMenuEntries[] PROGMEM = { g_SoundMenu0, g_SoundMenu1, g_SoundMenu2, g_SoundMenu3, g_SoundMenu4, g_SoundMenu5, g_SoundMenu6, g_SoundMenu7, g_SoundMenu8, g_SoundMenu9, g_SoundMenu10};

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t MenuSound::PrintMenuValue(void* userData, int8_t item, bool isSelected)
{
    MenuSound* self = static_cast<MenuSound*>(userData);
    
    while( g_Display.GetCursorX() < 11 ) g_Display.WriteData(' ');
//    g_Display.SetCursor(13, g_Display.GetCursorY());
    
    if ( item >= e_MenuFirstBeep && item <= e_MenuLastBeep )
    {
        uint16_t value = (isSelected) ? self->m_EditValue : Config::GetBeepLength(static_cast<BeepID::Enum>(item - e_MenuFirstBeep));
        return printf_P((isSelected) ? PSTR("[%d]mS") : PSTR("%dmS"), value * 5);
    }
    switch(item)
    {
        case e_MenuVolume:
            return printf_P((isSelected) ? PSTR("[%d]%%") : PSTR("%d%%"), Beeper::GetBuzzerVolume());
        case e_MenuFrequency:
            return printf_P((isSelected) ? PSTR("[%dHz]") : PSTR("%dHz"), CLOCK_FREQUENCY / MAIN_TIMER_PERIODE / 2 / (Beeper::GetBuzzerFrequency() + 1));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuSound::Initialize()
{
    g_Display.SetCursor(0, 0);
    printf_P(PSTR("Sound"));
    m_Menu.SetMenu(g_SoundMenuEntries, ARRAY_COUNT(g_SoundMenuEntries), PrintMenuValue, this, 1, 3);    
    
    Beeper::Mute(SoundCategory::e_Tactile, true);
    Beeper::Mute(SoundCategory::e_Progress, true);
    Beeper::Mute(SoundCategory::e_Alarm, true);    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuSound::Shutdown()
{
    Beeper::Mute(SoundCategory::e_Tactile, false);
    Beeper::Mute(SoundCategory::e_Progress, false);
    Beeper::Mute(SoundCategory::e_Alarm, false);    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuSound::HandleSelect(int8_t itemID, bool isEditing)
{
    if ( itemID >= e_MenuFirstBeep && itemID <= e_MenuLastBeep )
    {
        BeepID::Enum beepID = static_cast<BeepID::Enum>(itemID - e_MenuFirstBeep);
        
        if ( !isEditing ) {
            m_EditValue = Config::GetBeepLength(beepID);
            m_Menu.EditItem(true, -1);
        } else {
            Config::SetBeepLength(beepID, m_EditValue);
            m_Menu.EditItem(false, -1);
        }
        return;
    }
    switch(itemID)
    {
        case e_MenuVolume:
            if ( !isEditing ) {
                m_Menu.EditItem(true, Beeper::GetBuzzerVolume());
            } else {
                m_Menu.EditItem(false, -1);
                Config::SetBuzzerVolume(Beeper::GetBuzzerVolume());
            }
            break;
        case e_MenuFrequency:
            if ( !isEditing ) {
                m_Menu.EditItem(true, -1/*Beeper::GetBuzzerFrequency()*/);
            } else {
                m_Menu.EditItem(false, -1);
                Config::SetBuzzerFrequency(Beeper::GetBuzzerFrequency());
            }
            break;
        case e_MenuBack:
        case -1: // Cancel
            GlobalState::SetPrevState();
            break;
    }
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuSound::HandleEdit(int8_t itemID, int8_t value)
{
    if ( itemID >= e_MenuFirstBeep && itemID <= e_MenuLastBeep )
    {
        m_EditValue += value;
        Beeper::BeepTimed(m_EditValue);
        m_Menu.UpdateItem(itemID, -1);
    }
    else
    {
        switch(itemID)
        {
            case e_MenuVolume:
            {
                int8_t newValue = clamp<int8_t>(0, 100, Beeper::GetBuzzerVolume() + value * 10);
                Beeper::SetBuzzerVolume(newValue);
                Beeper::BeepTimed(Config::GetBeepLength(BeepID::e_Knob));
                m_Menu.UpdateItem(itemID, newValue);
                break;
            }                    
            case e_MenuFrequency:
            {
                uint8_t newValue = clamp<int16_t>(0, 255, I16(Beeper::GetBuzzerFrequency()) - value);
                Beeper::SetBuzzerFrequency(newValue);
                Beeper::BeepTimed(Config::GetBeepLength(BeepID::e_Knob));
                m_Menu.UpdateItem(itemID, -1);
                break;
            }
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuSound::Run(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_KeyDown:
        case EventID::e_KeyRepeat:
            switch( event.value )
            {
                case KbdKeyCodes::e_Start:
                {
                    int8_t itemID = m_Menu.GetSelectedItem();
                    if ( itemID >= e_MenuFirstBeep && itemID <= e_MenuLastBeep )
                    {
                        BeepID::Enum beepID = static_cast<BeepID::Enum>(itemID - e_MenuFirstBeep);
                        if (m_Menu.IsEditing()) {
                            Beeper::BeepTimed(m_EditValue);
                        } else {
                            Beeper::BeepTimed(Config::GetBeepLength(beepID));
                        }
                    }
                    else if ( itemID == e_MenuVolume || itemID == e_MenuFrequency )
                    {
                        Beeper::BeepTimed(800 / Beeper::TIME_PER_TICK);
                    }
                    return true;
                }                    
            }
            return m_Menu.HandleEvent(event);
        case EventID::e_MenuSelected:
            HandleSelect(event.value, m_Menu.IsEditing());
            return true;
        case EventID::e_MenuItemEdited:
            HandleEdit(m_Menu.GetSelectedItem(), event.value);
            return true;
        default:
            return m_Menu.HandleEvent(event);
    }
}
