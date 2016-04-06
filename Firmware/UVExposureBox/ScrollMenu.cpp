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

#include "ScrollMenu.h"
#include "Keyboard.h"
#include "Hardware.h"
#include "GlobalState.h"

#include "Misc/Display.h"
#include "Misc/Utils.h"

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ScrollMenu::SetMenu(const char* const * menu, uint8_t itemCount, ValuePrintCallback_t* printCallback, void* userData, uint8_t displayStart, uint8_t displayEnd)
{
    m_Items = menu;
    m_ItemCount = itemCount;
    m_PrintCallback = printCallback;
    m_UserData      = userData;
    m_DisplayStart  = displayStart;
    m_DisplayEnd    = displayEnd;
    m_HighlightedItem = 0;
    m_SelectedItem = -1;
    m_FirstVisibleItem = 0;
    m_Width = 0;
    for ( uint8_t i = 0 ; i < m_ItemCount ; ++i )
    {
        const char* text = (const char*)pgm_read_ptr_near(&m_Items[i]);
        uint8_t len = strlen_P(text);
        if ( len > m_Width )
        {
            m_Width = len;
        }
    }
    Print();
}    

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ScrollMenu::SelectItem( int8_t selection )
{
    m_HighlightedItem = clamp<int8_t>(0, m_ItemCount -1, selection);
    const int8_t displayLines = m_DisplayEnd - m_DisplayStart + 1;
    m_FirstVisibleItem = m_HighlightedItem - (displayLines >> 1);
    if ( m_FirstVisibleItem < 0 ) m_FirstVisibleItem = 0;
    else if ( m_ItemCount - m_FirstVisibleItem < displayLines ) m_FirstVisibleItem = m_ItemCount - displayLines;
    Print();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ScrollMenu::EditItem(bool doEdit, int8_t percentage)
{
    if ( doEdit )
    {
        m_SelectedItem = m_HighlightedItem;
        UpdateItem(m_SelectedItem, percentage);
    }
    else
    {
        m_SelectedItem = -1;
        Print();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ScrollMenu::UpdateItem(int8_t item, int8_t percentage)
{
    int8_t itemLine = GetItemScreenPos(item);
    PrintItem(item);

    if ( percentage >= 0 )
    {
        g_Display.PrintHorizontalBar( (itemLine != m_DisplayEnd) ? (itemLine + 1) : (itemLine - 1), percentage);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t ScrollMenu::GetItemScreenPos(int8_t item) const
{
    return item - m_FirstVisibleItem + m_DisplayStart;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ScrollMenu::PrintItem(int8_t item)
{
    int8_t displayLine = GetItemScreenPos(item);
    
    const char* text = (m_Items != nullptr) ? (const char*)pgm_read_ptr_near(&m_Items[item]) : nullptr;
    g_Display.SetCursor(0, displayLine);
    
    if ( m_SelectedItem == -1 && item == m_HighlightedItem ) {
        g_Display.WriteData('\x7E'); // Arrow right
    } else if ( item != 0 && displayLine == m_DisplayStart ) {
        g_Display.WriteData('\x06'); // Arrow up
    } else if ( item != m_ItemCount - 1 && displayLine == m_DisplayEnd ) {
        g_Display.WriteData('\x07'); // Arrow down
    } else {
        g_Display.WriteData(' ');
    }                
        
    if ( text != nullptr )
    {
        printf_P(text);
    }        
//    g_Display.WriteData((m_SelectedItem == -1 && item == m_HighlightedItem) ? '\x7F' : ' ');
        
    if ( m_PrintCallback != NULL && g_Display.GetCursorX() != DisplayLCD::WIDTH )
    {
        g_Display.WriteData(' ');
        m_PrintCallback(m_UserData, item, item == m_SelectedItem);
    }
    g_Display.ClearToEndOfLine();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ScrollMenu::Print()
{
    uint8_t lastVisibleItem = m_FirstVisibleItem + m_DisplayEnd - m_DisplayStart;
    if ( lastVisibleItem >= m_ItemCount ) lastVisibleItem = m_ItemCount - 1;
    for ( uint8_t i = m_FirstVisibleItem ; i <= lastVisibleItem ; ++i )
    {
        PrintItem(i);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ScrollMenu::HandleEvent(const Event& event)
{
    switch(event.type)
    {
        case EventID::e_KeyDown:
            switch(event.value)
            {
                case KbdKeyCodes::e_KnobButton:
                case KbdKeyCodes::e_Menu:
                    GlobalState::AddEvent(Event(EventID::e_MenuSelected, m_HighlightedItem));
                    return true;
                case KbdKeyCodes::e_Back:
                    GlobalState::AddEvent(Event(EventID::e_MenuSelected, -1));
                    return true;
            }
            break;
        case EventID::e_KnobTurned:
        {
            if ( m_SelectedItem == -1 )
            {
                SelectItem(m_HighlightedItem + event.value);
            }
            else
            {
                GlobalState::AddEvent(Event(EventID::e_MenuItemEdited, event.value));
            }
            return true;
        }            
        default:
            break;
    }
    return false;
}

