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
#include <string.h>

#include "TextInput.h"
#include "GlobalState.h"
#include "Beeper.h"

#include "Misc/Utils.h"
#include "Misc/Clock.h"

//static char g_SymbolMapping[] PROGMEM = { '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', , '`', '{', '|', '}', '\x7E', '\x7F' };
static const char g_SymbolMapping[] PROGMEM = { ':', ';', ',', '.', '+', '-', '!', '?', '(', ')', '[', ']', '{', '}', '<', '>', '\'', '"', '\\', '/', '_', '#', '$', '%', '&', '*', '=', '@', '|', '`', '^', '\x7E', '\x7F' }; // 33
static const uint8_t g_CharGroups[] PROGMEM = {26, 66, 106, 146, 164};

static uint8_t GetGroupStart(int8_t group) { return (group == 0) ? 0 : pgm_read_byte_near(&g_CharGroups[group - 1]); }
static uint8_t GetGroupSize(int8_t group ) { return (group == 0) ? pgm_read_byte_near(&g_CharGroups[0]) : (pgm_read_byte_near(&g_CharGroups[group]) - pgm_read_byte_near(&g_CharGroups[group - 1])); }

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

char CharIndexToChar(uint8_t index, bool upperCase)
{
    if ( index < 26 ) {
        return ((upperCase) ? 'A' : 'a') + index;   // Alpha
    } else if ( index < 36 ) {
        return '0' + index - 26;                    // Numeric
    } else if ( index < 69 ) {
        return pgm_read_byte_near(&g_SymbolMapping[index - 36]); // Symbol1
    } else if ( index < 164 ) {
        return index - 69 + 161;                    // Symbol2
    } else {
        return ' ';
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::Initialize()
{
    m_Buffer = nullptr;
    m_MaxLength = 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::BeginEdit(char* buffer, int8_t maxLength)
{
    m_Buffer = buffer;
    m_MaxLength = maxLength;
    m_CurrentLength = 0;
    m_CurrentRange = 0;
    m_Uppercase = true;
    m_CurrentChar = 0;
    
    g_Display.ClearDisplay();
    for ( int8_t i = 0 ; i < m_MaxLength && m_Buffer[i] != 0 ; ++i )
    {
        g_Display.WriteData(m_Buffer[i]);
        m_CurrentLength++;
    }
    SetCursorPos(m_CurrentLength);
    
    PrintCharSet();
    PrintCharSelectCursor(0, false);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::EndEdit()
{
    CursorOff();
    if ( m_CurrentLength < m_MaxLength ) m_Buffer[m_CurrentLength] = '\0';
    m_Buffer = nullptr;
    m_MaxLength = 0;
    m_CursorPos = 0;
    m_CurrentChar = 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool TextInput::IsEditing() const
{
    return m_Buffer !=  nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool TextInput::HandleEvent(const Event& event)
{
    if ( m_KnobDownTime != 0 && Clock::GetTime() > m_KnobDownTime + 300 )
    {
        Beeper::Beep(BeepID::e_LongPress);
        m_KnobDownTime = 0;

        int8_t lineLength = (GetGroupSize(m_CurrentRange) + 1) >> 1;
        if ( m_CurrentChar < GetGroupStart(m_CurrentRange) + lineLength )
        {
            SetCurrentChar(m_CurrentChar + lineLength);
        }
        else if ( m_CurrentChar >= GetGroupStart(m_CurrentRange) + lineLength )
        {
            SetCurrentChar(m_CurrentChar - lineLength);
        }
    }
    bool keyRepeat = false;
    switch(event.type)
    {
        case EventID::e_KeyRepeat:
            keyRepeat = true;
            // FALL THROUGH //
        case EventID::e_KeyDown:
            if ( !keyRepeat && event.value >= KbdKeyCodes::e_Preset1 && event.value <= KbdKeyCodes::e_Preset5 )
            {
                int8_t range = event.value - KbdKeyCodes::e_Preset1;
                if ( range != m_CurrentRange )
                {
                    SetRange(range);
                }
                else
                {
                    if ( m_CurrentRange == 0 ) {
                        SetUpperCase(!m_Uppercase);
                    }
                }
                return true;
            }

            switch( event.value )
            {
                case KbdKeyCodes::e_Preset5:
                    return true;
                case KbdKeyCodes::e_Preset6:
                    return true;
                case KbdKeyCodes::e_Preset7:
                    return true;
                case KbdKeyCodes::e_Preset8:
                    if ( !keyRepeat )
                    {
                        int8_t lineLength = (GetGroupSize(m_CurrentRange) + 1) >> 1;
                        if ( m_CurrentChar < GetGroupStart(m_CurrentRange) + lineLength ) {
                            SetCurrentChar(m_CurrentChar + lineLength);
                        } else if ( m_CurrentChar >= GetGroupStart(m_CurrentRange) + lineLength ) {
                            SetCurrentChar(m_CurrentChar - lineLength);
                        }
                    }
                    return true;
                case KbdKeyCodes::e_KnobButton:
                    if ( !keyRepeat ) {
                        m_KnobDownTime = Clock::GetTime();
                    }                        
                    return true;
                case KbdKeyCodes::e_Start:
                    InsertChar(' ');
                    if ( keyRepeat ) Beeper::Beep(BeepID::e_KeyRepeat);
                    return true;
                case KbdKeyCodes::e_Stop:
                    if ( m_CursorPos == m_CurrentLength - 1 || MoveCursor(-1) ) {
                        DeleteChar();
                    }
                    if ( keyRepeat ) Beeper::Beep(BeepID::e_KeyRepeat);
                    return true;
                case KbdKeyCodes::e_ComprUp:
                    MoveCursor(1);
                    if ( keyRepeat ) Beeper::Beep(BeepID::e_KeyRepeat);
                    return true;
                case KbdKeyCodes::e_ComprDown:
                    MoveCursor(-1);
                    if ( keyRepeat ) Beeper::Beep(BeepID::e_KeyRepeat);
                    return true;
                case KbdKeyCodes::e_Back:
                    if ( !keyRepeat ) {
                        GlobalState::AddEvent(Event(EventID::e_TextEditCanceled, 0));
                    }                        
                    return true;
                case KbdKeyCodes::e_Menu:
                    if ( !keyRepeat ) {
                        GlobalState::AddEvent(Event(EventID::e_TextEditDone, m_CursorPos));
                    }                        
                    return true;
            }
            return false;
        case EventID::e_KeyUp:
            switch( event.value )
            {
                case KbdKeyCodes::e_KnobButton:
                    if ( m_KnobDownTime != 0 ) {
                        InsertChar(CharIndexToChar(m_CurrentChar, m_Uppercase));
                    }
                    m_KnobDownTime = 0;
                    return true;
            }
            return false;                    
        case EventID::e_KnobTurned:
        {
            if ( -event.value > m_CurrentChar ) {
                SetCurrentChar(0);
            } else {
                SetCurrentChar(m_CurrentChar + event.value);
            }                
            
            return true;
        }            
        default:
            return false;
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool TextInput::MoveCursor(int8_t delta)
{
    return SetCursorPos(m_CursorPos + delta);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool TextInput::SetCursorPos(int8_t pos)
{
    if ( pos < 0 ) {
        pos = 0;
    } else if ( pos >= m_MaxLength ) {
        pos = m_MaxLength - 1;
    }
    
    if ( pos != m_CursorPos )
    {
        m_CursorPos = pos;
        g_Display.SetCursor(m_CursorPos, 0);        
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t TextInput::GetCharCursorPos(bool* pointDown) const
{
    int8_t lineLength = (GetGroupSize(m_CurrentRange) + 1) >> 1;
    int8_t pos = m_CurrentChar - GetGroupStart(m_CurrentRange);
    if ( pos < lineLength ) {
        if ( pointDown != nullptr ) {
            *pointDown = false;
        }            
    } else {
        if ( pointDown != nullptr ) {
            *pointDown = true;
        }            
        pos -= lineLength;
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::SetRange(int8_t range)
{
    if ( range != m_CurrentRange )
    {
        bool prevPointDown;
        int8_t prevCursorPos = GetCharCursorPos(&prevPointDown);
        
        m_CurrentRange = range;
        
        // Place char select cursor at the same location within the new range or at the end if out of range.
        int8_t lineLength = (GetGroupSize(m_CurrentRange) + 1) >> 1;
        m_CurrentChar = GetGroupStart(m_CurrentRange) + min<int8_t>(lineLength - 1, prevCursorPos);
        if ( prevPointDown ) m_CurrentChar += lineLength;
        
        bool pointDown;
        int8_t cursorPos = GetCharCursorPos(&pointDown);
        
        CursorOff();
        if (prevCursorPos != cursorPos) {
            g_Display.SetCursor(prevCursorPos, 2);
            g_Display.WriteData(' ');
        }
        PrintCharSelectCursor(cursorPos, pointDown);
        PrintCharSet(); // Will turn cursor on.
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::SetUpperCase(bool upperCase)
{
    m_Uppercase = !m_Uppercase;
    PrintCharSet();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::PrintCharSelectCursor(int8_t pos, bool down)
{
    g_Display.SetCursor(pos, 2);
    g_Display.WriteData((down) ? 7 : 6);
    g_Display.SetCursor(m_CursorPos, 0);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::SetCurrentChar(uint8_t curChar)
{
    if ( curChar >= GetGroupStart(ARRAY_COUNT(g_CharGroups)) ) {
        curChar = GetGroupStart(ARRAY_COUNT(g_CharGroups)) - 1;
    }
    if ( curChar != m_CurrentChar )
    {
        int8_t prevCursorPos = GetCharCursorPos(nullptr);
        int8_t prevRange = m_CurrentRange;
        m_CurrentChar = curChar;

        while(m_CurrentChar < GetGroupStart(m_CurrentRange) ) m_CurrentRange--;
        while(m_CurrentChar >= GetGroupStart(m_CurrentRange) + GetGroupSize(m_CurrentRange) ) m_CurrentRange++;
        
        if ( m_CurrentRange != prevRange ) {
            PrintCharSet();
        }            

        bool pointDown;
        int8_t cursorPos = GetCharCursorPos(&pointDown);

        CursorOff();        
        if (prevCursorPos != cursorPos) {
            g_Display.SetCursor(prevCursorPos, 2);
            g_Display.WriteData(' ');
        }
        PrintCharSelectCursor(cursorPos, pointDown);
        CursorOn();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::InsertChar(char character)
{
    if ( m_CurrentLength < m_MaxLength )
    {
        memmove(&m_Buffer[m_CursorPos + 1], &m_Buffer[m_CursorPos], m_CurrentLength - m_CursorPos);
        m_Buffer[m_CursorPos] = character;
        m_CurrentLength++;

        CursorOff();
        for (int8_t i = m_CursorPos ; i < m_CurrentLength ; ++i)
        {
            g_Display.WriteData(m_Buffer[i]);
        }
        if ( m_CursorPos < m_MaxLength - 1 ) {
            m_CursorPos++;
        }            
        g_Display.SetCursor(m_CursorPos, 0);
        CursorOn();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::DeleteChar()
{
    if ( m_CursorPos < m_CurrentLength )
    {
        memmove(&m_Buffer[m_CursorPos], &m_Buffer[m_CursorPos + 1], m_CurrentLength - m_CursorPos - 1);
        m_CurrentLength--;
        
        CursorOff();
        for ( int8_t i = m_CursorPos ; i < m_CurrentLength ; ++i ) {
            g_Display.WriteData(m_Buffer[i]);
        }
        g_Display.ClearToEndOfLine();
        g_Display.SetCursor(m_CursorPos, 0);
        CursorOn();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TextInput::PrintCharSet()
{
    uint8_t rangeStart = GetGroupStart(m_CurrentRange);
    int8_t  rangeSize  = GetGroupSize(m_CurrentRange);
    int8_t  lineLength = (rangeSize + 1) >> 1;

    CursorOff();
    g_Display.SetCursor(0, 1);    
    for (int8_t i = 0 ; i < rangeSize ; ++i )
    {
        if ( i == lineLength ) {
            g_Display.ClearToEndOfLine();
            g_Display.SetCursor(0, 3);
        }
        g_Display.WriteData(CharIndexToChar(rangeStart + i, m_Uppercase));
    }
    g_Display.ClearToEndOfLine();

    g_Display.SetCursor(m_CursorPos, 0);
    CursorOn();
}

