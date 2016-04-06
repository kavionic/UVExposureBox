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


#ifndef __VALUEEDITOR_H__
#define __VALUEEDITOR_H__

#include <avr/io.h>

struct Event;

class ValueEditor
{
public:
    typedef void ValuePrintCallback_t(int16_t);
    
    ValueEditor();
    
    void Setup(const char* label, int16_t minValue, int16_t maxValue, int16_t startValue, ValuePrintCallback_t* printCallback);
    
    void HandleEvent(const Event& event);
    
private:
    void Print();

    ValuePrintCallback_t* m_PrintCallback;
    
    int8_t  m_LabelWidth;
    int16_t m_MinValue;
    int16_t m_MaxValue;
    int16_t m_CurrentValue;
    
    // Disabled operators:
    ValueEditor( const ValueEditor &c );
    ValueEditor& operator=( const ValueEditor &c );
};

#endif //__VALUEEDITOR_H__
