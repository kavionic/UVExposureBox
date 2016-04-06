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


#ifndef __MENUABOUT_H__
#define __MENUABOUT_H__

#include <avr/io.h>

struct Event;

class MenuAbout
{
public:
    void Initialize();
    void Shutdown() {}
    bool Run(const Event& event);

private:
    void PrintLine(int8_t line);
    void Print();

    int8_t m_ScrollOffset;
};

#endif //__MENUABOUT_H__
