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

#include "MenuHome.h"
#include "Hardware.h"
#include "GlobalState.h"
#include "Config.h"
#include "PWMController.h"
#include "PanelController.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"

enum
{
    e_MenuAdvancedUpper,
    e_MenuAdvancedLower,
    e_MenuAdvancedCompressor,
    e_MenuAdvancedRename,
    e_MenuAdvancedBackup,
    e_MenuAdvancedRestore,
    e_MenuAdvancedSwap,
    e_MenuAdvancedCopy,
    e_MenuAdvancedUseAsCalibration
};

enum
{
    e_MenuSimpleTime,
    e_MenuSimpleUpper,
    e_MenuSimpleLower,
    e_MenuSimpleCompressor,
    e_MenuSimpleRename,
    e_MenuSimpleBackup,
    e_MenuSimpleRestore,
    e_MenuSimpleSwap,
    e_MenuSimpleCopy,
    e_MenuSimpleUseAsCalibration
};

enum
{
    e_FieldMinutes,
    e_FieldSeconds,
    e_FieldPower
};

static const char g_HomeMenu0[] PROGMEM = "Time:";
static const char g_HomeMenu1[] PROGMEM = "Upper:";
static const char g_HomeMenu2[] PROGMEM = "Lower:";
static const char g_HomeMenu3[] PROGMEM = "Compr:";
static const char g_HomeMenu4[] PROGMEM = "Rename...";
static const char g_HomeMenu5[] PROGMEM = "Backup...";
static const char g_HomeMenu6[] PROGMEM = "Restore...";
static const char g_HomeMenu7[] PROGMEM = "Swap...";
static const char g_HomeMenu8[] PROGMEM = "Copy...";
static const char g_HomeMenu9[] PROGMEM = "Set/Get calibration";
//                                        "12345678901234567890"
static const char* const g_HomeMenuEntriesSimple[] PROGMEM = { g_HomeMenu0, g_HomeMenu1, g_HomeMenu2, g_HomeMenu3, g_HomeMenu4, g_HomeMenu5, g_HomeMenu6, g_HomeMenu7, g_HomeMenu8, g_HomeMenu9};

static const char* const g_HomeMenuEntries[] PROGMEM = { g_HomeMenu1, g_HomeMenu2, g_HomeMenu3, g_HomeMenu4, g_HomeMenu5, g_HomeMenu6, g_HomeMenu7, g_HomeMenu8, g_HomeMenu9};


///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::Initialize()
{
    m_State = e_StateNormal;
    m_EditField = -1;
    m_Advanced = (Config::GetPresetFlags() & ExposurePreset::FLAG_ADVANCED) != 0;
    Config::GetPresetName(m_PresetName);
    
    m_TextEditor.Initialize();
    if ( m_Advanced ) {
        m_Menu.SetMenu(g_HomeMenuEntries, ARRAY_COUNT(g_HomeMenuEntries), PrintHomeMenuValue, this, 1, 3);
    } else {        
        m_Menu.SetMenu(g_HomeMenuEntriesSimple, ARRAY_COUNT(g_HomeMenuEntriesSimple), PrintHomeMenuValue, this, 1, 3);
    }
    UpdateStatusBar();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::UpdateStatusBar()
{
    g_Display.SetCursor(0, 0);
    if ( m_PresetName[0] != '\0' ) {
        printf_P(PSTR("%.20s"), m_PresetName);
    } else {
        printf_P(PSTR("Preset %d"), Config::GetSelectedPreset() + 1);
    }
    g_Display.ClearToEndOfLine();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::Run(const Event& event)
{
    switch(m_State)
    {
        case e_StateNormal:
            return HandleStateNormal(event);
        case e_StateRenaming:
            return HandleStateRenaming(event);
        case e_StateBackupAck:
        case e_StateRestoreAck:
            return HandleStateBackupRestoreAck(event);
        case e_StateCopyTargetSelect:
        case e_StateSwapTargetSelect:
            return HandleStateTargetSelect(event);
        case e_StateSetCalibration:
            return HandleStateSetCalibration(event);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::SetState(MenuHome::State state)
{
    if ( state != m_State )
    {
        m_State = state;
        switch(m_State)
        {
            case e_StateNormal:
                UpdateStatusBar();
                m_Menu.Print();
                break;
            case e_StateRenaming:
                m_TextEditor.BeginEdit(m_PresetName, sizeof(m_PresetName));
                break;
            case e_StateBackupAck:
            case e_StateRestoreAck:
                m_EditValue.i8 = 0;
                g_Display.ClearDisplay();
                g_Display.SetCursor(3, 0);
                if ( m_State == e_StateBackupAck ) {
                    printf_P(PSTR("BACKUP PRESET"));
                } else {
                    printf_P(PSTR("RESTORE PRESET"));
                }                    
                g_Display.SetCursor(0, 2);
                printf_P(PSTR("Are you sure?"));
                g_Display.SetCursor(14, 2);
                printf_P(PSTR("[NO] "));
                break;
            case e_StateCopyTargetSelect:
            case e_StateSwapTargetSelect:
                m_EditValue.i8 = Config::GetSelectedPreset();
                if ( m_EditValue.i8 != 7) ++m_EditValue.i8; else --m_EditValue.i8;
                
                g_Display.ClearDisplay();
                g_Display.SetCursor(3, 0);
                if ( m_State == e_StateBackupAck ) {
                    printf_P(PSTR("Copy preset"));
                } else {
                    printf_P(PSTR("Swap presets"));                    
                }
                PrintTargetPreset();
                break;
            case e_StateSetCalibration:
            {
                m_EditValue.i8x2.i8_2 = e_CalibrateCancel;
                uint32_t lowerRadiation = U32(Config::GetPresetPanelDuration(LightPanelID::e_Lower)) * Config::GetPresetPanelPower(LightPanelID::e_Lower) * Config::GetPanelCalibration(LightPanelID::e_Lower) / 100;
                uint32_t upperRadiation = U32(Config::GetPresetPanelDuration(LightPanelID::e_Upper)) * Config::GetPresetPanelPower(LightPanelID::e_Upper) * Config::GetPanelCalibration(LightPanelID::e_Upper) / 100;
                if ( lowerRadiation < upperRadiation ) {
                    m_EditValue.i8x2.i8_1 = -(100 - lowerRadiation * 100 / upperRadiation);
                } else if ( lowerRadiation > upperRadiation ) {
                    m_EditValue.i8x2.i8_1 = 100 - upperRadiation * 100 / lowerRadiation;
                } else {
                    m_EditValue.i8x2.i8_1 = 0;
                }
                PrintCalibrationMode();
                PrintCalibration();
                break;
            }                
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::HandleStateNormal(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_PresetSelected:
            Initialize();
            return false;
        case EventID::e_KeyDown:
            if ( HandleKeyDown(KbdKeyCodes::Enum(event.value)) ) {
                return true;
            } else {
                return m_Menu.HandleEvent(event);
            }
        case EventID::e_MenuSelected:
            if ( m_Advanced ) {
                HandleSelectAdvanced(event.value);
            } else {
                HandleSelectSimple(event.value);
            }
            return true;
        case EventID::e_MenuItemEdited:
        {
            int8_t selectedItem = m_Menu.GetSelectedItem();
            if ( m_Advanced ) {
                HandleEditAdvanced(selectedItem, event.value);
            } else {
                HandleEditSimple(selectedItem, event.value);
            }
            return true;
        }
        case EventID::e_CompressorUpdate:
             m_Menu.UpdateItem((m_Advanced) ? int8_t(e_MenuAdvancedCompressor) : int8_t(e_MenuSimpleCompressor),-1);
             return false;
        default:
            return m_Menu.HandleEvent(event);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::HandleStateRenaming(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_TextEditDone:
            m_State = e_StateNormal;
            m_TextEditor.EndEdit();
            Config::SetPresetName(m_PresetName);
            UpdateStatusBar();
            m_Menu.Print();
            return true;
        case EventID::e_TextEditCanceled:
            m_State = e_StateNormal;
            m_TextEditor.EndEdit();
            Config::GetPresetName(m_PresetName);
            UpdateStatusBar();
            m_Menu.Print();
            return true;
        default:
            return m_TextEditor.HandleEvent(event);
    }            
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::HandleStateBackupRestoreAck(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_KeyDown:
            switch(event.value)
            {
                case KbdKeyCodes::e_KnobButton:
                case KbdKeyCodes::e_Menu:
                    if (m_EditValue.i8 != 0 )
                    {
                        if ( m_State == e_StateBackupAck ) {
                            Config::CopyPresets(&g_EEPROM.presets.m_ExposurePresetBackups[Config::GetSelectedPreset()], &g_EEPROM.presets.m_ExposurePresets[Config::GetSelectedPreset()]);
                        } else {
                            Config::CopyPresets(&g_EEPROM.presets.m_ExposurePresets[Config::GetSelectedPreset()], &g_EEPROM.presets.m_ExposurePresetBackups[Config::GetSelectedPreset()]);
                            Config::GetPresetName(m_PresetName);
                        }
                    }
                    SetState(e_StateNormal);
                    return true;
                case KbdKeyCodes::e_Back:
                    SetState(e_StateNormal);
                    return true;
            }
            return false;
        case EventID::e_KnobTurned:
            g_Display.SetCursor(14, 2);
            if ( event.value < 0 ) {
                m_EditValue.i8 = 0;
                printf_P(PSTR("[NO] "));
            } else if ( event.value > 0 ) {
                m_EditValue.i8 = 1;
                printf_P(PSTR("[YES]"));
            }
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::HandleStateTargetSelect(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_KeyDown:
            switch(event.value)
            {
                case KbdKeyCodes::e_KnobButton:
                case KbdKeyCodes::e_Menu:
                    if ( m_State == e_StateCopyTargetSelect ) {
                        Config::CopyPresets(&g_EEPROM.presets.m_ExposurePresets[m_EditValue.i8], &g_EEPROM.presets.m_ExposurePresets[Config::GetSelectedPreset()]);
                    } else {
                        if ( m_EditValue.i8 != Config::GetSelectedPreset() ) {
                            Config::SwapPresets(&g_EEPROM.presets.m_ExposurePresets[m_EditValue.i8], &g_EEPROM.presets.m_ExposurePresets[Config::GetSelectedPreset()]);
                        } else {
                            Config::SwapPresets(&g_EEPROM.presets.m_ExposurePresets[m_EditValue.i8], &g_EEPROM.presets.m_ExposurePresetBackups[m_EditValue.i8]);
                        }
                        Config::GetPresetName(m_PresetName);
                    }
                    SetState(e_StateNormal);
                    return true;
                case KbdKeyCodes::e_Back:
                    SetState(e_StateNormal);
                    return true;
            }
            return false;
        case EventID::e_KnobTurned:
            if ( event.value < 0 ) {
                m_EditValue.i8 = clamp<int8_t>(0, 7, m_EditValue.i8 + event.value);
                if ( m_State == e_StateCopyTargetSelect && m_EditValue.i8 == Config::GetSelectedPreset() ) m_EditValue.i8 = (m_EditValue.i8 != 0) ? (m_EditValue.i8 - 1) : 1;
            } else if ( event.value > 0 ) {
                m_EditValue.i8 = clamp<int8_t>(0, 7, m_EditValue.i8 + event.value);
                if ( m_State == e_StateCopyTargetSelect && m_EditValue.i8 == Config::GetSelectedPreset() ) m_EditValue.i8 = (m_EditValue.i8 != 7) ? (m_EditValue.i8 + 1) : 6;
            }
            PrintTargetPreset();
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::HandleStateSetCalibration(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_KeyDown:
            switch(event.value)
            {
                case KbdKeyCodes::e_KnobButton:
                case KbdKeyCodes::e_Menu:
                {
                    // Calculate total radiation per panel based on time/power per panel and the current calibration setting.
                    uint32_t lowerRadiation = (U32(Config::GetPresetPanelDuration(LightPanelID::e_Lower)) * Config::GetPresetPanelPower(LightPanelID::e_Lower) * Config::GetPanelCalibration(LightPanelID::e_Lower) + 50) / 100;
                    uint32_t upperRadiation = (U32(Config::GetPresetPanelDuration(LightPanelID::e_Upper)) * Config::GetPresetPanelPower(LightPanelID::e_Upper) * Config::GetPanelCalibration(LightPanelID::e_Upper) + 50) / 100;
                    
                    switch(m_EditValue.i8x2.i8_2)
                    {
                        case e_CalibrateCancel:
                            break;
                        case e_CalibrateSet:
                        {
                            // Set both panels to 100% power and the time that will allow the highest radiating panel
                            // to give the same radiation as before. Then set the calibration value so the lowest
                            // radiating panel also give the same radiation as before.
                            if ( lowerRadiation > upperRadiation ) {
                                Config::SetPresetPanelDuration(LightPanelID::e_Lower, (lowerRadiation + 50) / 100);
                                Config::SetPresetPanelDuration(LightPanelID::e_Upper, (lowerRadiation + 50) / 100);
                            } else {
                                Config::SetPresetPanelDuration(LightPanelID::e_Lower, (upperRadiation + 50) / 100);
                                Config::SetPresetPanelDuration(LightPanelID::e_Upper, (upperRadiation + 50) / 100);
                            }
                            Config::SetPresetPanelPower(LightPanelID::e_Lower, 100);
                            Config::SetPresetPanelPower(LightPanelID::e_Upper, 100);
                            Config::SetPanelCalibration(m_EditValue.i8x2.i8_1);
                            break;
                        }                            
                        case e_CalibrateGet:
                        {
                            // Set the calibration value to 0 and then set the time of both panel so the highest radiating
                            // panel give the same radiation as before at 100% power. Then set the power of the lowest radiating
                            // panel so it will also give the same radiation as before.
                            if ( lowerRadiation > upperRadiation ) {
                                Config::SetPresetPanelDuration(LightPanelID::e_Lower, (lowerRadiation + 50) / 100);
                                Config::SetPresetPanelDuration(LightPanelID::e_Upper, (lowerRadiation + 50) / 100);
                                Config::SetPresetPanelPower(LightPanelID::e_Lower, 100);
                                Config::SetPresetPanelPower(LightPanelID::e_Upper, (upperRadiation * 100 + 50) / lowerRadiation);
                            } else {
                                Config::SetPresetPanelDuration(LightPanelID::e_Lower, (upperRadiation + 50) / 100);
                                Config::SetPresetPanelDuration(LightPanelID::e_Upper, (upperRadiation + 50) / 100);
                                Config::SetPresetPanelPower(LightPanelID::e_Lower, (lowerRadiation * 100 + 50) / upperRadiation);
                                Config::SetPresetPanelPower(LightPanelID::e_Upper, 100);
                            }
                            Config::SetPanelCalibration(0);
                            break;
                        }                            
                    }
                    SetState(e_StateNormal);
                    return true;
                }                    
                case KbdKeyCodes::e_Back:
                    SetState(e_StateNormal);
                    return true;
            }
            return false;
        case EventID::e_KnobTurned:
            m_EditValue.i8x2.i8_2 = clamp(0, e_CalibrateModeCount - 1, m_EditValue.i8x2.i8_2 + event.value);
            PrintCalibrationMode();
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::PrintTargetPreset()
{
    g_Display.SetCursor(0, 2);
    printf_P(PSTR("Target preset: [%d]"), m_EditValue.i8 + 1);
    g_Display.SetCursor(0, 3);
    if ( m_EditValue.i8 != Config::GetSelectedPreset() )
    {
        for ( int8_t i = 0 ; i < 20 ; ++i ) {
            char c = eeprom_read_byte(reinterpret_cast<const uint8_t*>(&g_EEPROM.presets.m_ExposurePresets[m_EditValue.i8].m_Name[i]));
            if ( c ) {
                g_Display.WriteData(c);
            } else {
                if ( i == 0 ) {
                    printf_P(PSTR("Preset %d"), m_EditValue.i8 + 1);
                }
                break;
            }
        }
    }
    else
    {
        printf_P(PSTR("  Swap with backup"));
    }
    g_Display.ClearToEndOfLine();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::PrintCalibrationMode()
{
    g_Display.SetCursor(0,0);
    
    printf_P(PSTR("Calibration "));
    switch(m_EditValue.i8x2.i8_2)
    {
        case e_CalibrateCancel:         printf_P(PSTR("[Cancel]")); break;
        case e_CalibrateSet:            printf_P(PSTR("[Set]"));    break;
        case e_CalibrateGet:            printf_P(PSTR("[Get]"));    break;
    }
    g_Display.ClearToEndOfLine();
}
    
///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::PrintCalibration()
{
    g_Display.SetCursor(0, 1);

    int8_t upper = Config::GetPanelCalibration(LightPanelID::e_Upper, m_EditValue.i8x2.i8_1);
    int8_t lower = Config::GetPanelCalibration(LightPanelID::e_Lower, m_EditValue.i8x2.i8_1);

    printf_P(PSTR("Lower "));
    PrintPercentage(lower);
    printf_P(PSTR("/Upper "));
    PrintPercentage(upper);

    g_Display.PrintHorizontalBar(2, upper, 0);
    g_Display.PrintHorizontalBar(3, lower, 1);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t MenuHome::PrintHomeMenuValue(void* userData, int8_t item, bool isSelected)
{
    MenuHome* self = static_cast<MenuHome*>(userData);
    bool advanced = self->m_Advanced;
    
    if ( !advanced && item == e_MenuSimpleTime )
    {
        const char* formatStr;
        uint16_t duration;
        
        if ( self->m_EditField == e_FieldMinutes ) {
            formatStr = PSTR("[%02d]:%02d");
            duration = self->m_EditValue.i16;
        } else if ( self->m_EditField == e_FieldSeconds ) {
            formatStr = PSTR("%02d:[%02d]");
            duration = self->m_EditValue.i16;
        } else {
            formatStr = PSTR("%02d:%02d");
            duration = Config::GetPresetPanelDuration(LightPanelID::e_Lower);
        }
        printf_P(formatStr, duration / 60, duration % 60 );
    }
    else if ( (!advanced && (item == e_MenuSimpleLower || item == e_MenuSimpleUpper)) || (advanced && (item == e_MenuAdvancedLower || item == e_MenuAdvancedUpper)) )
    {
        LightPanelID::Enum panelID;
        if ( advanced ) {
            panelID = (item == e_MenuAdvancedUpper) ? LightPanelID::e_Upper : LightPanelID::e_Lower;
        } else {
            panelID = (item == e_MenuSimpleUpper) ? LightPanelID::e_Upper : LightPanelID::e_Lower;            
        }
        const char* formatStr;
        if ( advanced )
        {
            uint16_t duration;
            
            if ( !isSelected /*self->m_EditField == -1*/ ) {
                formatStr = PSTR("%02d:%02d / ");
                duration = Config::GetPresetPanelDuration(panelID);
            } else if ( self->m_EditField == e_FieldMinutes ) {
                formatStr = PSTR("[%02d]:%02d/");
                duration = self->m_EditValue.i16;
            } else if ( self->m_EditField == e_FieldSeconds ) {
                formatStr = PSTR("%02d:[%02d]/");
                duration = self->m_EditValue.i16;
            } else {
                formatStr = PSTR("%02d:%02d/");
                duration = Config::GetPresetPanelDuration(panelID);
            }
            printf_P(formatStr, duration / 60, duration % 60 );
        }
        uint8_t power;
        if ( (advanced) ? (!isSelected || self->m_EditField != e_FieldPower) : !isSelected ) {
            power = Config::GetPresetPanelPower(panelID);
            formatStr = (power != 0) ? PSTR("%d%%") : PSTR("OFF");
        } else {
            power = self->m_EditValue.i8;
            formatStr = (power != 0) ? PSTR("[%d]%%") : PSTR("[OFF]");
        }            
        printf_P(formatStr, power );
        return 0;
    }
    else if ( (!advanced && item == e_MenuSimpleCompressor) || (advanced && item == e_MenuAdvancedCompressor) )
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
        return printf_P(PSTR("%u (%s)"), GlobalState::GetCompressorStep(), speedStr);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::UpdateTimer(uint16_t* timer, int8_t deltaMinutes, int8_t deltaSeconds)
{
    uint8_t minutes = *timer / 60;
    uint8_t seconds = *timer % 60;
    if ( deltaMinutes ) {
        minutes = (minutes + 100 + deltaMinutes) % 100;
    }
    if ( deltaSeconds ) {
        seconds = (seconds + 60 + deltaSeconds) % 60;
    }
    *timer = int16_t(minutes) * 60 + seconds;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool MenuHome::HandleKeyDown(KbdKeyCodes::Enum key)
{
    switch( key )
    {
        case KbdKeyCodes::e_Menu:
            GlobalState::SetState(MainState::e_MainMenu);
            return true;
        case KbdKeyCodes::e_Start:
            PanelController::StartRadiation();
            GlobalState::SetState(MainState::e_Exposure);
            return true;
        default:
            if ( key >= KbdKeyCodes::e_Preset1 && key <= KbdKeyCodes::e_Preset8 )
            {
                int8_t preset = key - KbdKeyCodes::e_Preset1;
                if ( preset != Config::GetSelectedPreset() ) {
                    Config::SetSelectedPreset(preset);
                    m_Advanced = (Config::GetPresetFlags() & ExposurePreset::FLAG_ADVANCED) != 0;
                } else {
                    m_Advanced = !m_Advanced;
                    uint8_t flags = Config::GetPresetFlags();
                    if ( m_Advanced ) {
                        flags |= ExposurePreset::FLAG_ADVANCED;
                    } else {        
                        flags &= uint8_t(~ExposurePreset::FLAG_ADVANCED);
                        
                        uint16 lowerDuration = Config::GetPresetPanelDuration(LightPanelID::e_Lower);
                        uint16 upperDuration = Config::GetPresetPanelDuration(LightPanelID::e_Upper);
                        uint32_t lowerRadiation = U32(lowerDuration) * Config::GetPresetPanelPower(LightPanelID::e_Lower);
                        uint32_t upperRadiation = U32(upperDuration) * Config::GetPresetPanelPower(LightPanelID::e_Upper);
                        if ( lowerDuration < upperDuration ) {
                            Config::SetPresetPanelDuration(LightPanelID::e_Lower, upperDuration);
                            Config::SetPresetPanelPower(LightPanelID::e_Lower, lowerRadiation / upperDuration);
                        } else {
                            Config::SetPresetPanelDuration(LightPanelID::e_Upper, lowerDuration);
                            Config::SetPresetPanelPower(LightPanelID::e_Upper, upperRadiation / lowerDuration);                            
                        }
                    }
                    Config::SetPresetFlags(flags);
                }
                if ( m_Advanced ) {
                    m_Menu.SetMenu(g_HomeMenuEntries, ARRAY_COUNT(g_HomeMenuEntries), PrintHomeMenuValue, this, 1, 3);
                } else {        
                    m_Menu.SetMenu(g_HomeMenuEntriesSimple, ARRAY_COUNT(g_HomeMenuEntriesSimple), PrintHomeMenuValue, this, 1, 3);
                }
                return true;
            }
            return false;
    }                    
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::HandleSelectSimple(int8_t itemIndex)
{
    switch( itemIndex )
    {
        case e_MenuSimpleTime:
            if ( m_EditField == -1 ) {
                m_EditValue.i16 = Config::GetPresetPanelDuration(LightPanelID::e_Lower);
                m_EditField = e_FieldMinutes;
                m_Menu.EditItem(true, -1);
            } else if ( m_EditField == e_FieldMinutes ) {
                m_EditField = e_FieldSeconds;
                m_Menu.UpdateItem(itemIndex, -1);
            } else {
                m_EditField = -1;
                Config::SetPresetPanelDuration(LightPanelID::e_Lower, m_EditValue.i16);
                Config::SetPresetPanelDuration(LightPanelID::e_Upper, m_EditValue.i16);
                m_Menu.EditItem(false, -1);
            }
            break;
        case e_MenuSimpleLower:
        case e_MenuSimpleUpper:
        {
            LightPanelID::Enum panelID = (itemIndex == e_MenuSimpleUpper) ? LightPanelID::e_Upper : LightPanelID::e_Lower;
            if ( m_Menu.GetSelectedItem() != itemIndex ) {
                m_EditValue.i8 = Config::GetPresetPanelPower(panelID);
                m_Menu.EditItem(true, m_EditValue.i8);
            } else {
                Config::SetPresetPanelPower(panelID, m_EditValue.i8);
                m_Menu.EditItem(false, -1);
            }
            break;
        }
        case e_MenuSimpleCompressor:
            if ( m_Menu.GetSelectedItem() != e_MenuSimpleCompressor ) {
                m_Menu.EditItem(true, GlobalState::GetCompressorSpeed());
            } else {
                m_Menu.EditItem(false, -1);
            }
            break;
        case e_MenuSimpleRename:
            SetState(e_StateRenaming);
            break;
        case e_MenuSimpleBackup:
            SetState(e_StateBackupAck);
            break;
        case e_MenuSimpleRestore:
            SetState(e_StateRestoreAck);
            break;
        case e_MenuSimpleCopy:
            SetState(e_StateCopyTargetSelect);
            break;
        case e_MenuSimpleSwap:
            SetState(e_StateSwapTargetSelect);
            break;
        case e_MenuSimpleUseAsCalibration:
            SetState(e_StateSetCalibration);
            break;
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::HandleSelectAdvanced(int8_t itemIndex)
{
    switch( itemIndex )
    {
        case e_MenuAdvancedLower:
        case e_MenuAdvancedUpper:
        {
            LightPanelID::Enum panelID = (itemIndex == e_MenuAdvancedUpper) ? LightPanelID::e_Upper : LightPanelID::e_Lower;
        
            if ( m_EditField == -1 ) {
                m_EditValue.i16 = Config::GetPresetPanelDuration(panelID);
                m_EditField = e_FieldMinutes;
                m_Menu.EditItem(true, -1);
            } else if ( m_EditField == e_FieldMinutes ) {
                m_EditField = e_FieldSeconds;
                m_Menu.UpdateItem(itemIndex, -1);
            } else if ( m_EditField == e_FieldSeconds ) {
                Config::SetPresetPanelDuration(panelID, m_EditValue.i16);
                m_EditValue.i8 = Config::GetPresetPanelPower(panelID);
                m_EditField = e_FieldPower;
                m_Menu.UpdateItem(itemIndex, m_EditValue.i8);
            } else {
                m_EditField = -1;
                Config::SetPresetPanelPower(panelID, m_EditValue.i8);
                m_Menu.EditItem(false, -1);
            }
            break;
        }
        case e_MenuAdvancedCompressor:
            if ( m_Menu.GetSelectedItem() != e_MenuAdvancedCompressor ) {
                m_Menu.EditItem(true, GlobalState::GetCompressorSpeed());
            } else {
                m_Menu.EditItem(false, -1);
            }                    
            break;
        case e_MenuAdvancedRename:
            SetState(e_StateRenaming);
            break;
        case e_MenuAdvancedBackup:
            SetState(e_StateBackupAck);
            break;
        case e_MenuAdvancedRestore:
            SetState(e_StateRestoreAck);
            break;
        case e_MenuAdvancedCopy:
            SetState(e_StateCopyTargetSelect);
            break;
        case e_MenuAdvancedSwap:
            SetState(e_StateSwapTargetSelect);
            break;
        case e_MenuAdvancedUseAsCalibration:
            SetState(e_StateSetCalibration);
            break;
//            Config::SwapPresets(&g_EEPROM.presets.m_ExposurePresetBackups[Config::GetSelectedPreset()], &g_EEPROM.presets.m_ExposurePresets[Config::GetSelectedPreset()]);
//            Initialize();
//            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::HandleEditSimple(int8_t itemIndex, int8_t value)
{
    if ( itemIndex == e_MenuSimpleTime )
    {
        if ( m_EditField == e_FieldMinutes ) {
            UpdateTimer(&m_EditValue.i16, value, 0);
            m_Menu.UpdateItem(itemIndex, -1);
        } else if ( m_EditField == e_FieldSeconds ) {
            UpdateTimer(&m_EditValue.i16, 0, value);
            m_Menu.UpdateItem(itemIndex, -1);
        }
    }
    else if ( itemIndex == e_MenuSimpleLower || itemIndex == e_MenuSimpleUpper )
    {
        m_EditValue.i8 = wrap<int8_t>(0, 100, m_EditValue.i8 + value);
        m_Menu.UpdateItem(itemIndex, m_EditValue.i8);
    }
    else if ( itemIndex == e_MenuSimpleCompressor )
    {
        GlobalState::SetCompressorSpeed(GlobalState::GetCompressorSpeed() + value);
        m_Menu.UpdateItem(itemIndex, GlobalState::GetCompressorSpeed());
    }    
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void MenuHome::HandleEditAdvanced(int8_t itemIndex, int8_t value)
{
    if ( itemIndex == e_MenuAdvancedLower || itemIndex == e_MenuAdvancedUpper )
    {
        if ( m_EditField == e_FieldMinutes ) {
            UpdateTimer(&m_EditValue.i16, value, 0);
            m_Menu.UpdateItem(itemIndex, -1);
        } else if ( m_EditField == e_FieldSeconds ) {
            UpdateTimer(&m_EditValue.i16, 0, value);
            m_Menu.UpdateItem(itemIndex, -1);
        } else {
            m_EditValue.i8 = wrap<int8_t>(0, 100, m_EditValue.i8 + value);
            m_Menu.UpdateItem(itemIndex, m_EditValue.i8);
        }
    }
    else if ( itemIndex == e_MenuAdvancedCompressor )
    {
        GlobalState::SetCompressorSpeed(GlobalState::GetCompressorSpeed() + value);
        m_Menu.UpdateItem(itemIndex, GlobalState::GetCompressorSpeed());
    }
}

