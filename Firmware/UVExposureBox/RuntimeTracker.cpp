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

#include <util/atomic.h>

#include "RuntimeTracker.h"
#include "Hardware.h"
#include "Config.h"
#include "PWMController.h"

#include "Misc/Utils.h"
#include "Misc/Clock.h"

int g_RuntimeWriteCount = 0;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

RuntimeTracker::RuntimeTracker()
{
    m_LastUpdateTime = 0;
    m_LastSaveTime = 0;
    m_UpdateSequence = 0;
    m_TotalTime = FindMostRecentTime(g_EEPROM.runtime.m_TotalRuntime, ARRAY_COUNT(g_EEPROM.runtime.m_TotalRuntime));
    for ( uint8_t i = 0 ; i < LightPanelID::e_Count ; ++i ) {
        m_PanelTime[i] = FindMostRecentTime(g_EEPROM.runtime.m_PanelRuntime[i], ARRAY_COUNT(g_EEPROM.runtime.m_PanelRuntime[i]));
        m_PanelNormalizedTime[i] = FindMostRecentTime(g_EEPROM.runtime.m_PanelNormalizedRuntime[i], ARRAY_COUNT(g_EEPROM.runtime.m_PanelNormalizedRuntime[i]));
    }
    m_CompressorTime = FindMostRecentTime(g_EEPROM.runtime.m_CompressorRuntime, ARRAY_COUNT(g_EEPROM.runtime.m_CompressorRuntime));
    m_CompressorNormalizedTime = FindMostRecentTime(g_EEPROM.runtime.m_CompressorNormalizedRuntime, ARRAY_COUNT(g_EEPROM.runtime.m_CompressorNormalizedRuntime));
}    

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void RuntimeTracker::Restart()
{
    m_LastSaveTime = Clock::GetTime();
    m_LastUpdateTime = 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void RuntimeTracker::Update(bool force)
{
    uint32_t time = Clock::GetTime();
    
    if ( !force && (time - m_LastUpdateTime < 2000) ) {
        return; // Only evaluate every 2 seconds.
    }
    m_LastUpdateTime = time;
    
//    if ( m_LastSaveTime == 0 ) m_LastSaveTime = time;
    
    uint32_t deltaTime = time - m_LastSaveTime;
    if ( force || deltaTime >= SAVE_TIME_INTERVAL_MS ) // Update total runtime every "SAVE_TIME_INTERVAL"
    {
        if ( force )
        {
            m_TotalTime += (deltaTime + 500) / 1000;
            m_LastSaveTime = time;
        }
        else
        {
            m_TotalTime += SAVE_TIME_INTERVAL_S;
            m_LastSaveTime += SAVE_TIME_INTERVAL_MS;
        }
        
        UpdateTime(g_EEPROM.runtime.m_TotalRuntime, ARRAY_COUNT(g_EEPROM.runtime.m_TotalRuntime), m_TotalTime);
    }
    
    switch(m_UpdateSequence++)
    {
        case 0:
            UpdateTime(g_EEPROM.runtime.m_PanelRuntime[LightPanelID::e_Lower], ARRAY_COUNT(g_EEPROM.runtime.m_PanelRuntime[LightPanelID::e_Lower]), GetPanelTime(LightPanelID::e_Lower, false));
            if ( !force) break;
        case 1:
            UpdateTime(g_EEPROM.runtime.m_PanelRuntime[LightPanelID::e_Upper], ARRAY_COUNT(g_EEPROM.runtime.m_PanelRuntime[LightPanelID::e_Upper]), GetPanelTime(LightPanelID::e_Upper, false));
            if ( !force) break;
        case 2:
            UpdateTime(g_EEPROM.runtime.m_PanelNormalizedRuntime[LightPanelID::e_Lower], ARRAY_COUNT(g_EEPROM.runtime.m_PanelNormalizedRuntime[LightPanelID::e_Lower]), GetPanelTime(LightPanelID::e_Lower, true));
            if ( !force) break;
        case 3:
            UpdateTime(g_EEPROM.runtime.m_PanelNormalizedRuntime[LightPanelID::e_Upper], ARRAY_COUNT(g_EEPROM.runtime.m_PanelNormalizedRuntime[LightPanelID::e_Upper]), GetPanelTime(LightPanelID::e_Upper, true));
            if ( !force) break;
        case 4:
            UpdateTime(g_EEPROM.runtime.m_CompressorRuntime, ARRAY_COUNT(g_EEPROM.runtime.m_CompressorRuntime), GetCompressorTime(false));
            if ( !force) break;
        case 5:
            UpdateTime(g_EEPROM.runtime.m_CompressorNormalizedRuntime, ARRAY_COUNT(g_EEPROM.runtime.m_CompressorNormalizedRuntime), GetCompressorTime(true));
            if ( !force) break;
        default:
            m_UpdateSequence = 0;
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint32_t RuntimeTracker::GetTotalTime() const
{
    return m_TotalTime + (Clock::GetTime() - m_LastSaveTime) / 1000;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint32_t RuntimeTracker::GetPanelTime(LightPanelID::Enum panelID, bool normalized) const
{
    uint32_t time;
    uint32_t startTime;
    uint8_t  power;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        time = (normalized) ? m_PanelNormalizedTime[panelID] : m_PanelTime[panelID];
        startTime = PWMController::GetLightPanelStartTime(panelID, &power);
    }
    if ( power )
    {
        startTime = (Clock::GetTime() - startTime) / 1000;
        if ( normalized ) {
            time += (startTime * power + 127) / 255;
        } else {
            time += startTime;            
        }
    }
    return time;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void RuntimeTracker::AddPanelTime(LightPanelID::Enum panelID, uint16_t seconds, uint8_t power)
{
    uint16_t normalizedTime = (uint32_t(seconds) * power + 127) / 255;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        m_PanelTime[panelID] += seconds;
        m_PanelNormalizedTime[panelID] += normalizedTime;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint32_t RuntimeTracker::GetCompressorTime(bool normalized) const
{
    uint32_t time;
    uint32_t startTime;
    uint8_t  power;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        time = (normalized) ? m_CompressorNormalizedTime : m_CompressorTime;
        startTime = PWMController::GetCompressorStartTime(&power);
    }
    if ( power )
    {
        startTime = (Clock::GetTime() - startTime) / 1000;
        if ( normalized ) {
            time += (startTime * power + 127) / 255;
        } else {
            time += startTime;
        }
    }
    return time;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void RuntimeTracker::AddCompressorTime(uint16_t seconds, uint8_t power)
{
    uint16_t normalizedTime = (uint32_t(seconds) * power + 127) / 255;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        m_CompressorTime += seconds;
        m_CompressorNormalizedTime += normalizedTime;
    }        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint32_t RuntimeTracker::FindMostRecentTime(const uint32_t* array, uint8_t arraySize)
{
    uint32_t largestTime = 0;
    for ( uint8_t i = 0 ; i < arraySize ; ++i )
    {
        uint32_t time = eeprom_read_dword(&array[i]);
        if ( time == 0xffffffff ) time = 0;
        if ( time > largestTime ) {
            largestTime = time;
        }
    }
    return largestTime;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void RuntimeTracker::UpdateTime(uint32_t* array, uint8_t arraySize, uint32_t value)
{
    if ( g_RuntimeWriteCount >= 1000 ) {
        return;
    }
    uint32_t largestTime = 0;
    uint8_t largestTimeIndex = 0;
    for ( uint8_t i = 0 ; i < arraySize ; ++i )
    {
        uint32_t time = eeprom_read_dword(&array[i]);
        if ( time == 0xffffffff ) time = 0;
        if ( time == value ) return;
        if ( time > largestTime ) {
            largestTime = time;
            largestTimeIndex = i;
        }
    }
    g_RuntimeWriteCount++;
    largestTimeIndex++;
    if ( largestTimeIndex >= arraySize ) largestTimeIndex = 0;
    eeprom_update_dword(&array[largestTimeIndex],value);
}
