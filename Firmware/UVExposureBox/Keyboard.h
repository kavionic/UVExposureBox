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


#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <avr/io.h>
#include <util/atomic.h>

#include "Misc/EventQueue.h"

namespace KbdKeyCodes
{
    enum Enum
    {
        e_Back       = 0,
        e_Menu       = 1,
        e_KnobButton = 2,
        e_Power      = 3,
        e_Preset1    = 4,
        e_Preset2    = 5,
        e_Preset3    = 6,
        e_Preset4    = 7,
        e_Preset5    = 8,
        e_Preset6    = 9,
        e_Preset7    = 10,
        e_Preset8    = 11,
        e_ComprDown  = 12,
        e_Start      = 13,
        e_Stop       = 14,
        e_ComprUp    = 15
    };
}

class Keyboard
{
public:    
    static void Initialize();
    static bool GetKeyState(uint8_t key);
    static const char* GetKeyNameP(uint8_t key);
    static void ScanKeyboard();
    static void UpdateKnob();

    static uint8_t GetKnobPos() { return s_KnobPos; }
/*    static int8_t GetAndClearRotaryDelta()
    {
        int8_t result;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            result = s_RotaryDelta;
            s_RotaryDelta = 0;
        }
        return result;
    }*/
private:
    static uint8_t  s_CurrentRow;
    static uint8_t  s_CurrentCol;
    static uint8_t  s_KeyIndex;
    static uint8_t  s_PrevKeyState[2];
    static uint8_t  s_KeyState[2];
    static int8_t   s_KnobPos;
    static int8_t   s_RotaryDelta;
    static uint8_t  s_PrevEncoderValue;
    static int8_t   s_LastPressedKey;
    static uint32_t s_NextRepeatTime;
};

#endif //__KEYBOARD_H__
