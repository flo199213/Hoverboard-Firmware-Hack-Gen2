/*
* This file is part of the hoverboard-firmware-hack-V2 project. The 
* firmware is used to hack the generation 2 board of the hoverboard.
* These new hoverboards have no mainboard anymore. They consist of 
* two Sensorboards which have their own BLDC-Bridge per Motor and an
* ARM Cortex-M3 processor GD32F130C8.
*
* Copyright (C) 2018 Florian Staeblein
* Copyright (C) 2018 Jakob Broemauer
* Copyright (C) 2018 Kai Liebich
* Copyright (C) 2018 Christoph Lehnert
*
* The program is based on the hoverboard project by Niklas Fauth. The 
* structure was tried to be as similar as possible, so that everyone 
* could find a better way through the code.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LED_H
#define LED_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

// Only slave has LED mechanism
#ifdef SLAVE

// Modes for RGB-LED operation
typedef enum
{
	LED_OFF = 0,
	LED_HSB = 1,
	LED_HSB_BLINK = 2,
	LED_HSB_FADE = 3,
	LED_HSB_STROBE = 4
} LED_PROGRAM;

#define COUNT_PROGRAMS 6	// Count of LED programs!!

//----------------------------------------------------------------------------
// Update RGB LED output with 16kHz
//----------------------------------------------------------------------------
void CalculateLEDPWM(void);

//----------------------------------------------------------------------------
// Update RGB LED program every 1ms
//----------------------------------------------------------------------------
void CalculateLEDProgram(void);

//----------------------------------------------------------------------------
// Sets/Gets LED program
//----------------------------------------------------------------------------
void SetRGBProgram(LED_PROGRAM Program);
LED_PROGRAM GetRGBProgram(void);

//----------------------------------------------------------------------------
// Sets/Gets hue from 0-764
//----------------------------------------------------------------------------
void SetHSBHue(uint16_t hue);
uint16_t GetHSBHue(void);

//----------------------------------------------------------------------------
// Sets/Gets saturation from 0-128
//----------------------------------------------------------------------------
void SetHSBSaturation(uint8_t saturation);
uint8_t GetHSBSaturation(void);

//----------------------------------------------------------------------------
// Sets/Gets brightness from 0-63
//----------------------------------------------------------------------------
void SetHSBBrightness(uint8_t brightnessVal);
uint8_t GetHSBBrightness(void);


//----------------------------------------------------------------------------
// Sets/Gets fading speed from 200-1000
//----------------------------------------------------------------------------
void SetSpeedFading(uint16_t speed);
uint16_t GetSpeedFading(void);

//----------------------------------------------------------------------------
// Sets/Gets blink speed from 700-2400
//----------------------------------------------------------------------------
void SetSpeedBlink(uint16_t speed);
uint16_t GetSpeedBlink(void);

//----------------------------------------------------------------------------
// Sets/Gets strobe speed from 0-1000
//----------------------------------------------------------------------------
void SetSpeedStrobe(uint16_t speed);
uint16_t GetSpeedStrobe(void);

#endif

#endif
