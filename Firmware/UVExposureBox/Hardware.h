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


#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <avr/io.h>
#include <avr/eeprom.h>

#include "Misc/Utils.h"

// PB0:	LCD_E
// PB1: LED_U (OC1A)
// PB2: LED_L (OC1B)
// PB3: BUZZER (OC2A)
// PB4: LCD_RS
// PB5: LCD_RW

// PC0: LCD_DB4
// PC1: LCD_DB5
// PC2: LCD_DB6
// PC3: LCD_DB7
// PC4: ROT_A (INT12)
// PC5: ROT_B (INT13)

// PD0:	KBD_0        (INT16)
// PD1: KBD_1        (INT17)
// PD2: KBD_2        (INT18)
// PD3: LCD_LED      (OC2B)
// PD4: KBD_3        (INT20)
// PD5: COMPRESSOR   (OC0B)
// PD6: LCD_CONTRAST (OC0A)
// PD7: KBD_4        (INT7)

#define PIN_LCD_E  PIN0_bm
#define PIN_LED_U  PIN1_bm
#define PIN_LED_L  PIN2_bm
#define PIN_BUZZER PIN3_bm
#define PIN_LCD_RS PIN4_bm
#define PIN_LCD_RW PIN5_bm

#define PORT_LCD_E   e_DigitalPortID_B
#define PORT_LED_U   e_DigitalPortID_B
#define PORT_LED_L   e_DigitalPortID_B
#define PORT_BUZZER  e_DigitalPortID_B
#define PORT_LCD_RS  e_DigitalPortID_B
#define PORT_LCD_RW  e_DigitalPortID_B

/*#define PIN_LCD_DB4	0
#define PORT_  e_DigitalPortID_C

#define PIN_LCD_DB5 1
#define PIN_LCD_DB6 2
#define PIN_LCD_DB7 3*/

#define PIN_ROT_TRIG_bp PIN4_bp
#define PIN_ROT_TRIG_bm PIN4_bm
#define IRQ_ROT_TRIG    PCINT12

#define PIN_ROT_DIR_bp PIN5_bp
#define PIN_ROT_DIR_bm PIN5_bm
#define IRQ_ROT_DIR    PCINT13

#define PORT_ROT  e_DigitalPortID_C
#define ROTARY_ENCODER_RESTING_POS 3

#define PIN_KBD_0        PIN0_bm
#define PIN_KBD_1        PIN1_bm
#define PIN_KBD_2        PIN2_bm
#define PIN_LCD_LED      PIN3_bm
#define PIN_KBD_3        PIN4_bm
#define PIN_COMPRESSOR   PIN5_bm
#define PIN_LCD_CONTRAST PIN6_bm
#define PIN_KBD_4        PIN7_bm

#define PORT_KBD          e_DigitalPortID_D
#define PORT_LCD_LED      e_DigitalPortID_D
#define PORT_COMPRESSOR   e_DigitalPortID_D
#define PORT_LCD_CONTRAST e_DigitalPortID_D

static const uint32_t CLOCK_FREQUENCY    = 20000000;
static const uint16_t MAIN_TIMER_PERIODE = 1000;

static const uint8_t TCCR0A_WAVEFORM         = BIT(WGM01, 1)  | BIT(WGM00, 1);   // Fast PWM A 0x00 -> 0xFF
static const uint8_t TCCR0A_LCD_CONTRAST_PWM = BIT(COM0A1, 1) | BIT(COM0A0, 1); // Inverting PWM A (LCD_CONTRAST)
static const uint8_t TCCR0A_COMPRESSOR_PWM   = BIT(COM0B1, 1) | BIT(COM0B0, 0); // Non inverting PWM B (COMPRESSOR)

static const uint8_t TCCR1A_WAVEFORM  = BIT(WGM11, 1)  | BIT(WGM10, 0);   // Fast PWM 0x0000 -> ICR1
static const uint8_t TCCR1A_LED_U_PWM = BIT(COM1A1, 1) | BIT(COM1A0, 0);  // Non inverting PWM A (LED_U)
static const uint8_t TCCR1A_LED_L_PWM = BIT(COM1B1, 1) | BIT(COM1B0, 0);  // Non inverting PWM B (LED_L)

static const uint8_t TCCR2A_WAVEFORM          = BIT(WGM21, 1)  | BIT(WGM20, 1);  // Fast PWM 0x00 -> 0xFF
static const uint8_t TCCR2A_BUZZER_PWM        = BIT(COM2A1, 1) | BIT(COM2A0, 0); // Non inverting PWM A (BUZZER)
static const uint8_t TCCR2A_LCD_BACKLIGHT_PWM = BIT(COM2B1, 1) | BIT(COM2B0, 0); // Non inverting PWM B (LCD_BACKLIGHT)


namespace LightPanelID
{
    enum Enum
    {
        e_Lower,
        e_Upper,
        e_Count
    };
};

struct LightPanel
{
    uint16_t m_Duration;
    uint8_t m_Intensity;
};


struct ExposurePreset
{
    static const uint8_t FLAG_ADVANCED = 0x01; // Allow individual timing of each panel.
    static const uint8_t NAME_LENGTH = 20;
    
    char       m_Name[NAME_LENGTH];
    LightPanel m_PanelSettings[LightPanelID::e_Count];
    uint8_t    m_Flags;
    uint8_t    m_CompressorStopDelay;
};

static const int8_t COMPRESSOR_SPEED_STEPS = 16;

namespace BeepID
{
    enum Enum
    {
        e_KeyPress,
        e_KeyRepeat,
        e_LongPress,
        e_Knob,
        e_Cancel,
        e_StartCountDown,
        e_FinishCountDown,
        e_StartRadiating,
        e_BeepCount
    };
}

class DisplayLCD;
extern DisplayLCD g_Display;

void panic();

void    SetBuzzerVolume(uint8_t volume);
uint8_t GetBuzzerVolume();
void    SetBuzzerFrequency(uint8_t frequency);
uint8_t GetBuzzerFrequency();


#endif /* HARDWARE_H_ */