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


#ifndef __MENUHOME_H__
#define __MENUHOME_H__

#include "ScrollMenu.h"
#include "TextInput.h"
#include "Keyboard.h"

struct Event;

class MenuHome
{
public:
    void Initialize();
    void Shutdown() {}

    bool Run(const Event& event);

private:
    enum State
    {
        e_StateNormal,
        e_StateRenaming,
        e_StateBackupAck,
        e_StateRestoreAck,
        e_StateCopyTargetSelect,
        e_StateSwapTargetSelect,
        e_StateSetCalibration
    };
    
    enum CalibrateMode
    {
        e_CalibrateCancel,
        e_CalibrateSet,
        e_CalibrateGet,
        e_CalibrateModeCount
    };
            
    void SetState(State state);
    void PrintTargetPreset();
    void PrintCalibrationMode();
    void PrintCalibration();

    static int8_t PrintHomeMenuValue(void* userData, int8_t item, bool isSelected);
    void UpdateStatusBar();
    void UpdateTimer(uint16_t* timer, int8_t deltaMinutes, int8_t deltaSeconds);
    bool HandleKeyDown(KbdKeyCodes::Enum key);
    void HandleSelectSimple(int8_t itemIndex);
    void HandleSelectAdvanced(int8_t itemIndex);
    void HandleEditSimple(int8_t itemIndex, int8_t value);
    void HandleEditAdvanced(int8_t itemIndex, int8_t value);
    
    bool HandleStateNormal(const Event& event);
    bool HandleStateRenaming(const Event& event);
    bool HandleStateBackupRestoreAck(const Event& event);
    bool HandleStateTargetSelect(const Event& event);
    bool HandleStateSetCalibration(const Event& event);
    
    State      m_State;
    ScrollMenu m_Menu;
    char       m_PresetName[sizeof(ExposurePreset::m_Name)];
    TextInput  m_TextEditor;
    bool       m_Advanced;
    int8_t     m_EditField;
    union {
        struct
        {
            uint8_t  i8_1;
            uint8_t  i8_2;
        } i8x2;
        uint8_t  i8;
        uint16_t i16;
    } m_EditValue;
};

#endif //__MENUHOME_H__
