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

#include "Config.h"

uint8_t Config::s_CurrentPreset;
int8_t  Config::s_CurrentPresetStorageSlot = -1;

uint8_t Config::s_KeyRepeatDelay;
uint8_t Config::s_KeyRepeatSpeed;
uint8_t Config::s_BeepLengths[BeepID::e_BeepCount];
uint8_t Config::s_AlarmCycles[8];
uint8_t Config::s_AlarmCycleCount;
int8_t  Config::s_PanelCalibration;

EEPROMContent g_EEPROM EEMEM;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::Initialize()
{
    uint8_t prev = eeprom_read_byte(&g_EEPROM.global.m_SelectedPreset[ARRAY_COUNT(g_EEPROM.global.m_SelectedPreset)-1]);
    for (uint8_t i = 0 ; i < ARRAY_COUNT(g_EEPROM.global.m_SelectedPreset) ; ++i )
    {
        uint8_t current = eeprom_read_byte(&g_EEPROM.global.m_SelectedPreset[i]);
        if ( (prev >> 3) == (current >> 3) )
        {
            s_CurrentPreset = prev & 7;
            s_CurrentPresetStorageSlot = (i != 0) ? (i-1) : (ARRAY_COUNT(g_EEPROM.global.m_SelectedPreset)-1);
            break;
        }
        prev = current;
    }
    if ( s_CurrentPreset >= 8 ) s_CurrentPreset = 0;
    
    s_KeyRepeatDelay   = eeprom_read_byte(&g_EEPROM.keyboard.m_RepeatDelay);
    s_KeyRepeatSpeed   = eeprom_read_byte(&g_EEPROM.keyboard.m_RepeatSpeed);
    s_AlarmCycleCount  = eeprom_read_byte(&g_EEPROM.sound.m_AlarmCycleCount);
    s_PanelCalibration = eeprom_read_byte(reinterpret_cast<uint8_t*>(&g_EEPROM.global.m_PanelCalibration));
    
    eeprom_read_block(s_BeepLengths, g_EEPROM.sound.m_BeepLengths, sizeof(s_BeepLengths));
    eeprom_read_block(s_AlarmCycles, g_EEPROM.sound.m_AlarmCycle, sizeof(s_AlarmCycles));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::Flush()
{
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SetBeepLength(BeepID::Enum beepID, uint8_t length)
{
    ASSERT_NOISR();
    if ( s_BeepLengths[beepID] != length )
    {
        s_BeepLengths[beepID] = length;
        eeprom_write_byte(&s_BeepLengths[beepID], length);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SetAlarmCycleCount(uint8_t count)
{
    ASSERT_NOISR();
    s_AlarmCycleCount = count;
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycleCount, count);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t Config::GetAlarmCycleLength(uint8_t cycle)
{
    return (cycle < ARRAY_COUNT(s_AlarmCycles)) ? s_AlarmCycles[cycle] : 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SetKeyRepeatDelay( uint8_t delay )
{
    ASSERT_NOISR();
    if ( delay != s_KeyRepeatDelay )
    {
        s_KeyRepeatDelay = delay;
        eeprom_write_byte(&g_EEPROM.keyboard.m_RepeatDelay, delay);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SetKeyRepeatSpeed( uint8_t speed )
{
    ASSERT_NOISR();
    if ( speed != s_KeyRepeatSpeed )
    {
        s_KeyRepeatSpeed = speed;
        eeprom_write_byte(&g_EEPROM.keyboard.m_RepeatSpeed, speed);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SetSelectedPreset(uint8_t preset)
{
    ASSERT_NOISR();
    
    if ( preset != s_CurrentPreset )
    {
        s_CurrentPreset = preset;
        uint8_t sequence = (eeprom_read_byte(&g_EEPROM.global.m_SelectedPreset[s_CurrentPresetStorageSlot++]) >> 3) + 1;
    
        if ( s_CurrentPresetStorageSlot >= int8_t(ARRAY_COUNT(g_EEPROM.global.m_SelectedPreset)) ) s_CurrentPresetStorageSlot = 0;
    
        eeprom_update_byte(&g_EEPROM.global.m_SelectedPreset[s_CurrentPresetStorageSlot], (sequence << 3) | s_CurrentPreset);
        GlobalState::AddEvent(Event(EventID::e_PresetSelected, preset));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SwapPresets(ExposurePreset* preset1, ExposurePreset* preset2)
{
    for ( uint8_t i = 0 ; i < sizeof(ExposurePreset) ; ++i )
    {
        uint8_t tmp = eeprom_read_byte(reinterpret_cast<uint8_t*>(preset1) + i);
        eeprom_update_byte(reinterpret_cast<uint8_t*>(preset1) + i, eeprom_read_byte(reinterpret_cast<uint8_t*>(preset2) + i));
        eeprom_update_byte(reinterpret_cast<uint8_t*>(preset2) + i, tmp);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::CopyPresets(ExposurePreset* dest, const ExposurePreset* source)
{
    for ( uint8_t i = 0 ; i < sizeof(ExposurePreset) ; ++i )
    {
        eeprom_update_byte(reinterpret_cast<uint8_t*>(dest) + i, eeprom_read_byte(reinterpret_cast<const uint8_t*>(source) + i));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t Config::GetPanelCalibration()
{
    return s_PanelCalibration;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Config::SetPanelCalibration(int8_t value)
{
    ASSERT_NOISR();
    if ( value != s_PanelCalibration )
    {
        s_PanelCalibration = value;
        eeprom_write_byte(reinterpret_cast<uint8_t*>(&g_EEPROM.global.m_PanelCalibration), value);
    }        
}
