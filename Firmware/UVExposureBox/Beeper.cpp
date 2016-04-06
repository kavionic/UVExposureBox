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


#include "Beeper.h"

uint8_t Beeper::s_TimerSkipCount;
uint8_t Beeper::g_BeepEndTime;
uint8_t Beeper::g_AlarmCycleCount;
uint8_t Beeper::g_AlarmCycle;
uint8_t Beeper::s_BuzzerFrequency;
uint8_t Beeper::s_MuteFlags;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::Mute(SoundCategory::Enum category, bool mute)
{
    if ( mute ) {
        s_MuteFlags |= BIT8(category, 1);
    } else {
        s_MuteFlags &= U8(~BIT8(category, 1));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool Beeper::IsMuted(SoundCategory::Enum category)
{
    return (s_MuteFlags & BIT8(category, 1)) != 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::SetMuteFlags(uint8_t flags)
{
    s_MuteFlags = flags;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t Beeper::GetMuteFlags()
{
    return s_MuteFlags;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::Beep( BeepID::Enum beepID )
{
    if ( beepID == BeepID::e_Cancel )
    {
        if ( IsMuted(SoundCategory::e_Alarm) ) {
            return;
        }
    }
    else if ( beepID == BeepID::e_StartCountDown || beepID == BeepID::e_FinishCountDown || beepID == BeepID::e_StartRadiating )
    {
        if ( IsMuted(SoundCategory::e_Progress) ) {
            return;
        }
    }
    else if ( IsMuted(SoundCategory::e_Tactile) )
    {
        return;
    }
    BeepTimed(Config::GetBeepLength(beepID));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::BeepTimed(uint8_t duration)
{
    DigitalPort::SetAsOutput(PORT_BUZZER, PIN_BUZZER);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        s_TimerSkipCount = TIME_PER_TICK;
        g_AlarmCycleCount = 0;
        g_BeepEndTime = duration;
    }        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::Alarm()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        s_TimerSkipCount = TIME_PER_TICK;
        g_BeepEndTime = 1;
        g_AlarmCycle = 0;
        g_AlarmCycleCount = Config::GetAlarmCycleCount() << 1;
    }        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::SetBuzzerVolume( uint8_t volume )
{
    OCR2A = PercentToByte(volume);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t Beeper::GetBuzzerVolume()
{
    return ByteToPercent(OCR2A);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Beeper::SetBuzzerFrequency( uint8_t frequency )
{
    s_BuzzerFrequency = frequency;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t Beeper::GetBuzzerFrequency()
{
    return s_BuzzerFrequency;
}

