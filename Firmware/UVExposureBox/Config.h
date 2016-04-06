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


#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>
#include <avr/eeprom.h>

#include "Hardware.h"
#include "GlobalState.h"

#define ASSERT_NOISR() do { if ( (SREG & BIT(7,1)) == 0 ) { panic(); } } while(false)

struct EEPROMContent
{
    static const uint8_t VERSION = 2;
    struct Global
    {
        uint8_t        m_Version;
        uint8_t        m_StandbyState; // 1 when on, 0 if standby. If 1 when powered up it will activate, if not it will go to standby-
        uint8_t        m_SelectedPreset[33]; // 33 slots for wear balancing.
        uint8_t        m_CompressorSpeeds[COMPRESSOR_SPEED_STEPS];
        int8_t         m_PanelCalibration;
    } global;
    struct Display
    {
        uint8_t        m_LCDBacklight;
        uint8_t        m_LCDContrast;        
    } display;
    struct Sound
    {
        uint8_t        m_Volume;
        uint8_t        m_Frequency;
        uint8_t        m_BeepLengths[BeepID::e_BeepCount];
        uint8_t        m_AlarmCycle[8];
        uint8_t        m_AlarmCycleCount;        
    } sound;
    struct Keyboard
    {
        uint8_t m_RepeatDelay;
        uint8_t m_RepeatSpeed;
    } keyboard;
    uint8_t m_Padding[53/*277*/];
    
    struct Presets
    {
        ExposurePreset m_ExposurePresetBackups[8];    
        ExposurePreset m_ExposurePresets[8];
    } presets;
    struct Runtime
    {
        uint32_t       m_TotalRuntime[64];
        uint32_t       m_PanelRuntime[LightPanelID::e_Count][8];
        uint32_t       m_PanelNormalizedRuntime[LightPanelID::e_Count][8];
        uint32_t       m_CompressorRuntime[8];
        uint32_t       m_CompressorNormalizedRuntime[8];
    } runtime;
};

#if !defined(VA_VERSION)
extern EEPROMContent g_EEPROM EEMEM;
#else
extern EEPROMContent g_EEPROM; // Never defined, but works with visual assist.
#endif

class Config
{
public:
    static void Initialize();
    static void Flush();

    // Display:
    static uint8_t GetLCDBacklight()              { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.display.m_LCDBacklight); }
    static void    SetLCDBacklight(uint8_t value) { ASSERT_NOISR(); eeprom_update_byte(&g_EEPROM.display.m_LCDBacklight, value); }

    static uint8_t GetLCDContrast()              { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.display.m_LCDContrast); }
    static void    SetLCDContrast(uint8_t value) { ASSERT_NOISR(); eeprom_update_byte(&g_EEPROM.display.m_LCDContrast, value); }

    // Sound:
    static uint8_t GetBuzzerVolume()              { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.sound.m_Volume); }
    static void    SetBuzzerVolume(uint8_t value) { ASSERT_NOISR(); return eeprom_update_byte(&g_EEPROM.sound.m_Volume, value); }

    static uint8_t GetBuzzerFrequency()              { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.sound.m_Frequency); }
    static void    SetBuzzerFrequency(uint8_t value) { ASSERT_NOISR(); return eeprom_update_byte(&g_EEPROM.sound.m_Frequency, value); }
        
    static uint8_t GetBeepLength(BeepID::Enum beepID) { return s_BeepLengths[beepID]; }
    static void    SetBeepLength(BeepID::Enum beepID, uint8_t length);

    static uint8_t GetAlarmCycleCount() { return s_AlarmCycleCount; }
    static void    SetAlarmCycleCount(uint8_t count);

    static uint8_t GetAlarmCycleLength(uint8_t cycle);
    
    // Keyboard:
    static uint8_t GetKeyRepeatDelay() { return s_KeyRepeatDelay; }
    static void    SetKeyRepeatDelay(uint8_t delay);

    static uint8_t GetKeyRepeatSpeed() { return s_KeyRepeatSpeed; }
    static void    SetKeyRepeatSpeed(uint8_t speed);
        
    // Radiation presets:
    static uint8_t GetSelectedPreset() { return s_CurrentPreset; }
    static void    SetSelectedPreset(uint8_t preset);
    
    static void    GetPresetName(char* buffer)       { ASSERT_NOISR(); eeprom_read_block(buffer, &g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_Name, sizeof(g_EEPROM.presets.m_ExposurePresets[0].m_Name)); }
    static void    SetPresetName(const char* buffer) { ASSERT_NOISR(); eeprom_update_block(buffer, &g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_Name, sizeof(g_EEPROM.presets.m_ExposurePresets[0].m_Name)); }
    
    static uint8_t GetPresetFlags()                  { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_Flags); }
    static void    SetPresetFlags(uint8_t flags)     { ASSERT_NOISR(); eeprom_update_byte(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_Flags, flags); }
    
    static uint8_t GetPresetCompressorStopDelay()              { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_CompressorStopDelay); }
    static void    SetPresetCompressorStopDelay(uint8_t delay) { ASSERT_NOISR(); eeprom_update_byte(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_CompressorStopDelay, delay); }
    
    static uint16_t GetPresetPanelDuration(LightPanelID::Enum panelID)                     { ASSERT_NOISR(); return eeprom_read_word(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_PanelSettings[panelID].m_Duration); }
    static void     SetPresetPanelDuration(LightPanelID::Enum panelID, uint16_t duration)  { ASSERT_NOISR(); eeprom_update_word(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_PanelSettings[panelID].m_Duration, duration); }

    static uint8_t  GetPresetPanelPower(LightPanelID::Enum panelID)                { ASSERT_NOISR(); return eeprom_read_byte(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_PanelSettings[panelID].m_Intensity); }
    static void     SetPresetPanelPower(LightPanelID::Enum panelID, uint8_t power) { ASSERT_NOISR(); eeprom_update_byte(&g_EEPROM.presets.m_ExposurePresets[GetSelectedPreset()].m_PanelSettings[panelID].m_Intensity, power); }

    static void     SwapPresets(ExposurePreset* preset1, ExposurePreset* preset2);        
    static void     CopyPresets(ExposurePreset* dest, const ExposurePreset* source);

    // Misc:
    static int8_t   GetPanelCalibration();
    static void     SetPanelCalibration(int8_t value);
    
    static uint8_t GetPanelCalibration(LightPanelID::Enum panelID, int8_t rawValue)
    {
        if ( panelID == LightPanelID::e_Lower ) {
            return (rawValue >= 0) ? 100 : (100 + rawValue);
        } else {
            return (rawValue <= 0) ? 100 : (100 - rawValue);
        }
    }
    static uint8_t GetPanelCalibration(LightPanelID::Enum panelID)
    {
        return GetPanelCalibration(panelID, GetPanelCalibration());
    }    
private:
    static uint8_t s_CurrentPreset;
    static int8_t  s_CurrentPresetStorageSlot;

    static uint8_t s_KeyRepeatDelay;
    static uint8_t s_KeyRepeatSpeed;
    
    static uint8_t s_BeepLengths[BeepID::e_BeepCount];
    static uint8_t s_AlarmCycles[8];
    static uint8_t s_AlarmCycleCount;
    
    static int8_t  s_PanelCalibration;
};



#endif // CONFIG_H_