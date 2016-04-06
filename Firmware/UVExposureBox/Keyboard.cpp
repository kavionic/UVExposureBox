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

#include "Keyboard.h"
#include "Hardware.h"
#include "GlobalState.h"
#include "Config.h"
#include "Beeper.h"

#include "Misc/Utils.h"
#include "Misc/DigitalPort.h"
#include "Misc/SpinTimer.h"
#include "Misc/Clock.h"

uint8_t  Keyboard::s_CurrentRow;
uint8_t  Keyboard::s_CurrentCol = 1;
uint8_t  Keyboard::s_KeyIndex;
uint8_t  Keyboard::s_PrevKeyState[2];
uint8_t  Keyboard::s_KeyState[2];
int8_t   Keyboard::s_KnobPos;
int8_t   Keyboard::s_RotaryDelta;
uint8_t  Keyboard::s_PrevEncoderValue = ROTARY_ENCODER_RESTING_POS;
int8_t   Keyboard::s_LastPressedKey = -1;
uint32_t Keyboard::s_NextRepeatTime;

static const uint8_t g_KeyPins[] PROGMEM = {PIN_KBD_0, PIN_KBD_1, PIN_KBD_2, PIN_KBD_3, PIN_KBD_4};
static const uint8_t KBD_PIN_COUNT = sizeof(g_KeyPins) / sizeof(g_KeyPins[0]);


static const char g_KeyName0[] PROGMEM = "BACK";
static const char g_KeyName1[] PROGMEM = "MENU";
static const char g_KeyName2[] PROGMEM = "KNOB";
static const char g_KeyName3[] PROGMEM = "POWR";
static const char g_KeyName4[] PROGMEM = "PRE1";
static const char g_KeyName5[] PROGMEM = "PRE2";
static const char g_KeyName6[] PROGMEM = "PRE3";
static const char g_KeyName7[] PROGMEM = "PRE4";
static const char g_KeyName8[] PROGMEM = "PRE5";
static const char g_KeyName9[] PROGMEM = "PRE6";
static const char g_KeyName10[] PROGMEM = "PRE7";
static const char g_KeyName11[] PROGMEM = "PRE8";
static const char g_KeyName12[] PROGMEM = "CPRD";
static const char g_KeyName13[] PROGMEM = "STRT";
static const char g_KeyName14[] PROGMEM = "STOP";
static const char g_KeyName15[] PROGMEM = "CPRU";

static const char* const g_KeyNames[] PROGMEM = { g_KeyName0, g_KeyName1, g_KeyName2, g_KeyName3, g_KeyName4, g_KeyName5, g_KeyName6, g_KeyName7, g_KeyName8, g_KeyName9, g_KeyName10, g_KeyName11, g_KeyName12, g_KeyName13, g_KeyName14, g_KeyName15};


///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Keyboard::Initialize()
{
    for ( uint8_t i = 0 ; i < KBD_PIN_COUNT ; ++i )
    {
        const uint8_t pin = BIT(pgm_read_byte_near(&g_KeyPins[i]), 1);
        DigitalPort::EnablePullup(PORT_KBD, pin);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool Keyboard::GetKeyState( uint8_t key )
{
    return (s_KeyState[key >> 3] & BIT(key & 7, 1)) != 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

const char* Keyboard::GetKeyNameP(uint8_t key)
{
    return (key < ARRAY_COUNT(g_KeyNames)) ? reinterpret_cast<const char*>(pgm_read_ptr_near(&g_KeyNames[key])) : reinterpret_cast<const char*>(NULL);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

static uint8_t GetPinMask(uint8_t pinIndex)
{
    return pgm_read_byte_near(&g_KeyPins[pinIndex]);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Keyboard::ScanKeyboard()
{
    if ( s_CurrentCol != s_CurrentRow )
    {
        uint8_t keyByte = s_KeyIndex >> 3;
        uint8_t keyBit  = s_KeyIndex & 7;
        uint8_t keyMask = BIT(keyBit, 1);
        if ( !(DigitalPort::Get(PORT_KBD) & GetPinMask(s_CurrentCol)) )
        {
            // Pressed:
            if ( (s_PrevKeyState[keyByte] & keyMask) != 0 && (s_KeyState[keyByte] & keyMask) == 0 )
            {
                s_KeyState[keyByte] |= keyMask;
                s_LastPressedKey = s_KeyIndex;
                s_NextRepeatTime = Clock::GetTime() + (uint16_t(Config::GetKeyRepeatDelay()) << 2);
                GlobalState::AddEvent(Event(EventID::e_KeyDown, s_KeyIndex));

                Beeper::Beep(BeepID::e_KeyPress);

            }
            s_PrevKeyState[keyByte] |= keyMask;
        }
        else
        {
            // Not pressed:
            if ( (s_PrevKeyState[keyByte] & keyMask) == 0 && (s_KeyState[keyByte] & keyMask) != 0 )
            {
                s_KeyState[keyByte] &= ~keyMask;
                if ( s_KeyIndex == s_LastPressedKey ) s_LastPressedKey = -1;
                GlobalState::AddEvent(Event(EventID::e_KeyUp, s_KeyIndex));
            }
            s_PrevKeyState[keyByte] &= ~keyMask;
        }
        s_KeyIndex++;
    }
    s_CurrentCol++;
    
    if ( s_CurrentCol >= KBD_PIN_COUNT )
    {
        uint8_t outPin = GetPinMask(s_CurrentRow);
        
        DigitalPort::SetAsInput(PORT_KBD, outPin);
        DigitalPort::EnablePullup(PORT_KBD, outPin);
        s_CurrentCol = 1;
        s_CurrentRow++;
        if ( s_CurrentRow >= KBD_PIN_COUNT )
        {
            s_CurrentRow = 0;
            s_KeyIndex = 0;
        }
        outPin = GetPinMask(s_CurrentRow);

        DigitalPort::SetLow(PORT_KBD, outPin);
        DigitalPort::SetAsOutput(PORT_KBD, outPin);
    }
    if ( s_LastPressedKey != -1 && (s_NextRepeatTime - Clock::GetTime()) <= 0 )
    {
        s_NextRepeatTime = Clock::GetTime() + (uint16_t(Config::GetKeyRepeatSpeed()) << 2);
        GlobalState::AddEvent(Event(EventID::e_KeyRepeat, s_LastPressedKey));
    }

    bool knobBeep = false;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if ( s_RotaryDelta )
        {
            Event* peekEvent = GlobalState::PeekEvent();
            if ( peekEvent != nullptr && peekEvent->type == EventID::e_KnobTurned )
            {
                peekEvent->value += s_RotaryDelta;
            } else {
                GlobalState::AddEvent(Event(EventID::e_KnobTurned, s_RotaryDelta));
            }
            s_RotaryDelta = 0;
            knobBeep = true;
        }
    }
    if ( knobBeep ) Beeper::Beep(BeepID::e_Knob);    
}

///////////////////////////////////////////////////////////////////////////////
/// Called from the pin-change interrupt associated with the two pins connected
/// to the rotary encoder. The relative position is determined by looking at
/// the current and the previous state of the encoder. The encoder goes through
/// 4 transitions for each detention. If the delta is larger den |1| when the
/// encoder reach the resting position the "detention delta" is updated based
/// on the sign of the high-res delta and the high-res delta is reset. This
/// ensures that the state is kept in sync with the encoder even if some pulses
/// is missed.
///////////////////////////////////////////////////////////////////////////////

void Keyboard::UpdateKnob()
{
    // Map encoder matrix. Maps the current and previous encoder value to motion delta.
    // The encoder sequence is (3, 1, 0, 2). Any invalid transitions map to '0' to be ignored.
    static const int8_t encoderMatrix[] PROGMEM = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
        
    uint8 portVal = DigitalPort::Get(PORT_ROT);
    uint8_t  encoderValue = ((portVal & PIN_ROT_TRIG_bm) >> PIN_ROT_TRIG_bp) | ((portVal & PIN_ROT_DIR_bm) >> (PIN_ROT_DIR_bp) << 1);

    s_PrevEncoderValue = ((s_PrevEncoderValue << 2) | encoderValue) & 0x0f;
    int8_t delta = pgm_read_byte_near(&encoderMatrix[s_PrevEncoderValue]);

    s_KnobPos += delta;
    
    if ( encoderValue == ROTARY_ENCODER_RESTING_POS )
    {
        if ( s_KnobPos > 1 ) {
            ++s_RotaryDelta;
            s_KnobPos = 0;
        } else if ( s_KnobPos < -1 ) {
            --s_RotaryDelta;
            s_KnobPos = 0;
        }
    }    
}
