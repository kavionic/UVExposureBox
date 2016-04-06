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

#include "PWMController.h"
#include "Config.h"
#include "GlobalState.h"
#include "RuntimeTracker.h"

#include "Misc/Utils.h"
#include "Misc/Clock.h"

uint32_t PWMController::s_PanelStartTime[LightPanelID::e_Count];
uint32_t PWMController::s_CompressorStartTime;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PWMController::SetLCDBacklight(uint8_t intensity)
{
    OCR2B = PercentToByte(intensity);

    if ( intensity != 0 ) {
        TCCR2A |= TCCR2A_LCD_BACKLIGHT_PWM;
        OCR2B = PercentToByte(intensity);
    } else {
        TCCR2A &= ~TCCR2A_LCD_BACKLIGHT_PWM;
        OCR2B = 0;
    }
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t PWMController::GetLCDBacklight()
{
    return ByteToPercent(OCR2B);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PWMController::SetLCDContrast(uint8_t intensity)
{
    if ( intensity != 0 ) {
        TCCR0A |= TCCR0A_LCD_CONTRAST_PWM;
        OCR0A = PercentToByte(intensity);
    } else {
        TCCR0A &= ~TCCR0A_LCD_CONTRAST_PWM;
        OCR0A = 0;
    }
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t PWMController::GetLCDContrast()
{
    return ByteToPercent(OCR0A);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PWMController::SetLightPanel(LightPanelID::Enum panelID, uint8_t intensity)
{
 //   intensity = U16(intensity) * Config::GetPanelCalibration(panelID) / 100;
    uint16_t power = uint16_t(intensity) * 20;
    if ( panelID == LightPanelID::e_Upper )
    {
        if ( intensity == 0 ) {
            TCCR1A &= ~TCCR1A_LED_U_PWM; // Disable PWM
        } else {
            TCCR1A |= TCCR1A_LED_U_PWM; // Enable PWM
        }
        if ( OCR1A != power )
        {
            uint32_t time = Clock::GetTime();
            if ( OCR1A != 0 ) {
                GlobalState::GetRuntimeTracker().AddPanelTime(panelID, (time - s_PanelStartTime[panelID] + 500) / 1000, uint32_t(OCR1A) * 255 / 2000);
            }
            s_PanelStartTime[panelID] = (power) ? time : 0;
            OCR1A = power; // LED_U
        }
    }
    else
    {
        if ( intensity == 0 ) {
            TCCR1A &= ~TCCR1A_LED_L_PWM;
        } else {
            TCCR1A |= TCCR1A_LED_L_PWM;
        }
        if ( OCR1B != power )
        {
            uint32_t time = Clock::GetTime();
            if ( OCR1B != 0 ) {
                GlobalState::GetRuntimeTracker().AddPanelTime(panelID, (time - s_PanelStartTime[panelID] + 500) / 1000, uint32_t(OCR1B) * 255 / 2000);
            }
            s_PanelStartTime[panelID] = (power) ? time : 0;
            OCR1B = power; // LED_L
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t PWMController::GetLightPanel(LightPanelID::Enum panelID)
{
    if ( panelID == LightPanelID::e_Upper ) {
        return uint16_t(OCR1A) / 20; // LED_U
    } else {
        return uint16_t(OCR1B) / 20; // LED_L
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint32_t PWMController::GetLightPanelStartTime(LightPanelID::Enum panelID, uint8_t* power)
{
    uint32_t time;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        time = s_PanelStartTime[panelID];
        *power = uint32_t((panelID == LightPanelID::e_Lower) ? OCR1B : OCR1A) * 255 / 2000;
    }
    return time;            
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PWMController::SetCompressor(uint8_t speed)
{
    uint8_t power;
    if ( speed != 0 ) {
        TCCR0A |= TCCR0A_COMPRESSOR_PWM;
        power = PercentToByte(speed);
    } else {
        TCCR0A &= ~TCCR0A_COMPRESSOR_PWM;
        power = 0;
    }
    
    if ( power != OCR0B )
    {
        uint32_t time = Clock::GetTime();
        if ( OCR0B != 0 ) {
            GlobalState::GetRuntimeTracker().AddCompressorTime((time - s_CompressorStartTime + 500) / 1000, OCR0B);
        }
        s_CompressorStartTime = time;
        OCR0B = power;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8_t PWMController::GetCompressor()
{
    return ByteToPercent(OCR0B);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint32_t PWMController::GetCompressorStartTime(uint8_t* power)
{
    uint32_t time;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        time = s_CompressorStartTime;
        *power = OCR0B;
    }
    return time;
}
