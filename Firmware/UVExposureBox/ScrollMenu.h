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


#ifndef __SCROLLMENU_H__
#define __SCROLLMENU_H__

#include <avr/io.h>

struct Event;

class ScrollMenu
{
public:
    typedef int8_t ValuePrintCallback_t(void*, int8_t, bool);
    
//    ScrollMenu();
        
    void SetMenu(const char* const * menu, uint8_t itemCount, ValuePrintCallback_t* printCallback, void* userData, uint8_t displayStart, uint8_t displayEnd );
    void EditItem(bool doEdit, int8_t percentage);
    void   SelectItem(int8_t selection);
    int8_t GetSelectedItem() const { return m_SelectedItem; }
    bool IsEditing() const { return m_SelectedItem != -1; }
    void UpdateItem(int8_t item, int8_t percentage);
    void Print();
    
    int8_t GetItemScreenPos(int8_t item) const;
    
    bool HandleEvent(const Event& event);
        
private:
    void PrintItem(int8_t item);

    const char* const * m_Items;
    int8_t             m_ItemCount;
    uint8_t             m_DisplayStart:4;
    uint8_t             m_DisplayEnd:4;
    int8_t             m_Width;
    int8_t             m_FirstVisibleItem;
    int8_t             m_HighlightedItem;
    int8_t             m_SelectedItem;
    
    ValuePrintCallback_t* m_PrintCallback;
    void*                 m_UserData;
    // Disabled operators:
//    ScrollMenu( const ScrollMenu &c );
    ScrollMenu& operator=( const ScrollMenu &c );
};

#endif //__SCROLLMENU_H__
