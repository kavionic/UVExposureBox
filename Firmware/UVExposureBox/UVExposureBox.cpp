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


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#include "Hardware.h"
#include "Config.h"
#include "PWMController.h"
#include "PanelController.h"
#include "GlobalState.h"
#include "Beeper.h"

#include "Misc/Utils.h"
#include "Misc/DigitalPort.h"
#include "Misc/Clock.h"

//static DigitalPort g_DisplayDataPort(e_DigitalPortID_C);
//static DigitalPort g_DisplayCtrlPort(e_DigitalPortID_B);

#define DISPLAY_DATA_PORT e_DigitalPortID_C
#define DISPLAY_CTRL_PORT e_DigitalPortID_B

#define SPINTIMER_TIMERL TCNT0
#define SPINTIMER_FREQ 20000000
//#define SPINTIMER_FREQ (20000000 / 8)


#define DISPLAY_REGISTER_SELECT PIN_LCD_RS
#define DISPLAY_RW              PIN_LCD_RW
#define DISPLAY_ENABLE          PIN_LCD_E

#define DISPLAY_DATA_0          PIN0_bm
#define DISPLAY_DATA_1          PIN1_bm
#define DISPLAY_DATA_2          PIN2_bm
#define DISPLAY_DATA_3          PIN3_bm

#include "Misc/Display.h"
#include "Misc/Display.cpp"
#include "Misc/SpinTimer.cpp"
#include "Misc/Clock.cpp"
#include "Misc/Utils.cpp"

#include "Keyboard.h"

DisplayLCD g_Display;
static volatile bool g_IsTimer1IRQRunning;
static volatile uint8_t g_Timer1IRQSkipCount = 0;
static uint16_t g_Timer1TickCounter = 100;
static uint8_t  g_Timer1Tick500 = 5;
static uint8_t  g_Timer1Tick1000 = 2;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

ISR(PCINT1_vect)
{
    Keyboard::UpdateKnob();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////


ISR(TIMER1_OVF_vect)
{
    Beeper::UpdateHF();
    
    ++g_Timer1IRQSkipCount;
    if ( g_Timer1IRQSkipCount < CLOCK_FREQUENCY / 1000 / MAIN_TIMER_PERIODE ) {
        return;
    }
    if ( g_IsTimer1IRQRunning ) {
        return;
    }
    g_IsTimer1IRQRunning = true;
    g_Timer1IRQSkipCount -= CLOCK_FREQUENCY / 1000 / MAIN_TIMER_PERIODE;
    sei();
    
    if ( !(--g_Timer1TickCounter) )
    {
        g_Timer1TickCounter = 100;
        GlobalState::AddEvent(Event(EventID::e_TimeTick100, 0));
        if ( !(--g_Timer1Tick500) )
        {
            g_Timer1Tick500 = 5;
            GlobalState::AddEvent(Event(EventID::e_TimeTick500, 0));
            if ( !(--g_Timer1Tick1000) )
            {
                g_Timer1Tick1000 = 2;
                GlobalState::AddEvent(Event(EventID::e_TimeTick1000, 0));
            }            
        }
    }
    Clock::IncrementTime();

    PanelController::UpdateRadiation();
    Keyboard::ScanKeyboard();
    Beeper::UpdateLF();
    
    uint16_t time = TCNT1;
//    if ( Clock::GetTime() % 2000 == 0 ) GlobalState::s_KeyScanTime = 0;
    if ( time > GlobalState::s_KeyScanTime ) GlobalState::s_KeyScanTime = time;
    
    g_IsTimer1IRQRunning = false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

static int DisplayPutchar(char c, FILE *stream)
{
    g_Display.PrintString(&c, 1, 1);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ResetConfig()
{
    // Global:
    eeprom_update_byte(&g_EEPROM.global.m_Version, EEPROMContent::VERSION);
    eeprom_update_byte(&g_EEPROM.global.m_StandbyState, 0);
    for ( uint8_t i = 0 ; i < ARRAY_COUNT(g_EEPROM.global.m_SelectedPreset) ; ++i ) {
        eeprom_update_byte(&g_EEPROM.global.m_SelectedPreset[i], (i == 0) ? 0 : ((i-1)<<3));
    }        
    eeprom_update_byte(&g_EEPROM.global.m_CompressorSpeeds[0], 30);
    eeprom_update_byte(&g_EEPROM.global.m_CompressorSpeeds[1], 50);
    eeprom_update_byte(&g_EEPROM.global.m_CompressorSpeeds[2], 70);
    eeprom_update_byte(&g_EEPROM.global.m_CompressorSpeeds[3], 0);
    eeprom_update_byte(reinterpret_cast<uint8_t*>(&g_EEPROM.global.m_PanelCalibration), 0);

    // Display:
    eeprom_update_byte(&g_EEPROM.display.m_LCDBacklight, 50);
    eeprom_update_byte(&g_EEPROM.display.m_LCDContrast, 75);

    // Sound:
    eeprom_update_byte(&g_EEPROM.sound.m_Volume, 100);
    eeprom_update_byte(&g_EEPROM.sound.m_Frequency, 8);
    
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_KeyPress], 10 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_KeyRepeat], 5 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_LongPress], 20 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_Knob], 5 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_Cancel], 20 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_StartCountDown], 25 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_FinishCountDown], 50 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_StartRadiating], 200 / 5);

    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycleCount, 20);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[0], 250 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[1], 125 / 5);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[2], 70  / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[3], 125  / 5);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[4], 70  / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[5], 125  / 5);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[6], 70  / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[7], 125  / 5);


    eeprom_update_byte(&g_EEPROM.keyboard.m_RepeatDelay, 70);
    eeprom_update_byte(&g_EEPROM.keyboard.m_RepeatDelay, 25);

    // Presets:
    for ( uint8_t i = 0 ; i < ARRAY_COUNT(g_EEPROM.presets.m_ExposurePresets) ; ++i )
    {
        eeprom_update_byte(reinterpret_cast<uint8_t*>(&g_EEPROM.presets.m_ExposurePresets[i].m_Name[0]), 0);
        for ( int8_t j = 0 ; j < LightPanelID::e_Count ; ++j )
        {
            eeprom_update_word(&g_EEPROM.presets.m_ExposurePresets[i].m_PanelSettings[j].m_Duration, ((i & 3) + 1) * 30); // (i<=4) ? 120 : 0);
            eeprom_update_byte(&g_EEPROM.presets.m_ExposurePresets[i].m_PanelSettings[j].m_Intensity, (i < 4 || !(j & 1)) ? 100 : 0);
        }            
        eeprom_update_byte(&g_EEPROM.presets.m_ExposurePresets[i].m_Flags, 0);
        eeprom_update_byte(&g_EEPROM.presets.m_ExposurePresets[i].m_CompressorStopDelay, 5);
    }
}

void panic()
{
    g_Display.SetCursor(0,0);
    printf_P(PSTR("PANIC!"));
    cli();
    for(;;);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int main(void)
{
    // Setup timer 0:
    TCCR0A = TCCR0A_WAVEFORM;
    TCCR0B = (0<<CS02) | (0<<CS01) | (1<<CS00); // Clock div: 1

    // Setup timer 1:
    TCCR1A = TCCR1A_WAVEFORM;
    TCCR1B = BIT(WGM13, 1)  | BIT(WGM12, 1) | BIT(CS12, 0) | BIT(CS11, 0) | BIT(CS10, 1); // Clock div: 1
    ICR1 = MAIN_TIMER_PERIODE; // 20MHz / 20000 = 1KHz LED panel PWM frequency and timer execution
    
    // Setup timer 2:
    TCCR2A = TCCR2A_WAVEFORM; // | TCCR2A_BUZZER_PWM;
//    TCCR2B = BIT(CS22,1) | BIT(CS21,0) | BIT(CS20,0); // Clock div: 64
    TCCR2B = BIT(CS22,0) | BIT(CS21,0) | BIT(CS20,1); // Clock div: 1
	
//    OCR0A = 190; // LCD_CONTRAST

//    OCR1A = 0; // LED_U
//    OCR1B = 0; // LED_L
    
//    OCR2A = 128; // BUZZER
//    OCR2B = 128; // LCD_BACKLIGHT

    DigitalPort::SetAsOutput(PORT_LCD_CONTRAST, PIN_LCD_CONTRAST);
    DigitalPort::SetHigh(PORT_LCD_CONTRAST, PIN_LCD_CONTRAST);
    DigitalPort::SetAsOutput(PORT_LCD_LED, PIN_LCD_LED);

    DigitalPort::SetAsOutput(PORT_LED_L, PIN_LED_L);
    DigitalPort::SetLow(PORT_LED_L, PIN_LED_L);

    DigitalPort::SetAsOutput(PORT_LED_U, PIN_LED_U);
    DigitalPort::SetLow(PORT_LED_U, PIN_LED_U);

    //    DigitalPort::SetAsOutput(PORT_BUZZER, PIN_BUZZER);
    DigitalPort::SetAsOutput(PORT_COMPRESSOR, PIN_COMPRESSOR);
    //	DigitalPort::SetHigh(PORT_LCD_CONTRAST, PIN_LCD_CONTRAST);

    if ( eeprom_read_byte(&g_EEPROM.global.m_Version) != EEPROMContent::VERSION ) {
        ResetConfig();
    }    
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_KeyPress], 10 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_KeyRepeat], 5 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_LongPress], 20 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_Knob], 5 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_Cancel], 20 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_StartCountDown], 25 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_FinishCountDown], 50 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_BeepLengths[BeepID::e_StartRadiating], 200 / 5);

    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycleCount, 20);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[0], 250 / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[1], 125 / 5);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[2], 70  / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[3], 125  / 5);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[4], 70  / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[5], 125  / 5);
    
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[6], 70  / 5);
    eeprom_update_byte(&g_EEPROM.sound.m_AlarmCycle[7], 125  / 5);
	
    Config::Initialize();
    

    // Setup interrupts:

    // Timer 1 interrupt:
    TIMSK1 = BIT(TOIE1, 1);

    // Pin Change for quadrature encoder.
    PCMSK1 = BIT(IRQ_ROT_TRIG, 1) | BIT(IRQ_ROT_DIR, 1);
    PCICR = BIT(PCIE2, 0) | BIT(PCIE1, 1) | BIT(PCIE0, 0); // Enable pin-change 8-14

//    SpinTimer::SleepMS(500);

    sei();

    for ( int8_t j = 0 ; j < 2 ; ++j ) {
    Beeper::SetBuzzerVolume(50);
    for (int8_t i = 10 ; i > 0 ; --i )
    {
        Beeper::SetBuzzerFrequency(i);
        Beeper::BeepTimed(100);
        SpinTimer::SleepMS(10);
    }        
    for (int8_t i = 2 ; i <= 10 ; ++i )
    {
        Beeper::SetBuzzerFrequency(i);
        Beeper::BeepTimed(100);
        SpinTimer::SleepMS(10);
    }
    }    
    Beeper::BeepTimed(0);
	
    g_Display.InitializeDisplay();
    g_Display.EnableDisplay(true);
    
    g_Display.WriteCustomCharBegin(6);
    g_Display.WriteCustomCharRow(0b00100);
    g_Display.WriteCustomCharRow(0b01010);
    g_Display.WriteCustomCharRow(0b10001);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);

    g_Display.WriteCustomCharBegin(7);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b00000);
    g_Display.WriteCustomCharRow(0b10001);
    g_Display.WriteCustomCharRow(0b01010);
    g_Display.WriteCustomCharRow(0b00100);
    
    g_Display.SetCursor(0,0);

    PWMController::SetLCDBacklight(eeprom_read_byte(&g_EEPROM.display.m_LCDBacklight));
    PWMController::SetLCDContrast(eeprom_read_byte(&g_EEPROM.display.m_LCDContrast));

    Beeper::SetBuzzerFrequency(Config::GetBuzzerFrequency());
    Beeper::SetBuzzerVolume(Config::GetBuzzerVolume());
    
    static FILE mystdout;
    fdev_setup_stream(&mystdout, DisplayPutchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;

    Keyboard::Initialize();

    DigitalPort::EnablePullup(PORT_ROT, PIN_ROT_DIR_bm | PIN_ROT_TRIG_bm);

    GlobalState::Initialize();

    GlobalState::Run();
    return 0;
}