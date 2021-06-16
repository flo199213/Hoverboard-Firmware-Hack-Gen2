/*
* This file is part of the Hoverboard Utility Gateway System (HUGS) project. 
*
* The HUGS project goal is to enable Hoverboards, or Hoverboard drive components 
* to be re-purposed to provide low-cost mobility to other systems, such
* as assistive devices for the disabled, general purpose robots or other
* labor saving devices.
*
* Copyright (C) 2020 Phil Malone
* Copyright (C) 2018 Florian Staeblein
* Copyright (C) 2018 Jakob Broemauer
* Copyright (C) 2018 Kai Liebich
* Copyright (C) 2018 Christoph Lehnert
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
*/

#ifndef BLDC_H
#define BLDC_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

#define SPEED_MODE_PF				0
#define SPEED_MODE_STEP			1
#define SPEED_MODE_DUAL			2
#define DEFAULT_SPEED_MODE	SPEED_MODE_PF
#define DEFAULT_MAX_STEP_SPEED 200

//----------------------------------------------------------------------------
// Set motor enable
//----------------------------------------------------------------------------
void SetEnable(FlagStatus setEnable);

//----------------------------------------------------------------------------
// Set speed -100 to 100
//----------------------------------------------------------------------------
void SetSpeed(int16_t setPwm);

//----------------------------------------------------------------------------
// Set power -100 to 100
//----------------------------------------------------------------------------
void SetPower(int16_t setPwm);

//----------------------------------------------------------------------------
// Set pwm -1000 to 1000
//----------------------------------------------------------------------------
void SetPWM(int16_t setPwm);

//----------------------------------------------------------------------------
// Get pwm -1000 to 1000
//----------------------------------------------------------------------------
int16_t GetPWM(void);

//----------------------------------------------------------------------------
// Get pwm -1000 to 1000
//----------------------------------------------------------------------------
void	  CalculateSpeed(void);
int16_t GetSpeed(void);

//----------------------------------------------------------------------------
// Get position in mm
//----------------------------------------------------------------------------
int32_t GetPosition(void);

//----------------------------------------------------------------------------
// Calculation-Routine for BLDC => calculates with 32kHz
//----------------------------------------------------------------------------
void CalculateBLDC(void);

//----------------------------------------------------------------------------
// Calculate output power based on closed loop control
//----------------------------------------------------------------------------
int16_t	runPID(void) ;

//----------------------------------------------------------------------------
// Determine Absolute values
//----------------------------------------------------------------------------
int16_t	abs16(int16_t value) ;
int32_t	abs32(int32_t value) ;

//----------------------------------------------------------------------------
// Determine the current commutation position (0-5)
//----------------------------------------------------------------------------
uint8_t	hallToPos(void) ;

//----------------------------------------------------------------------------
// Set the correct  y b g phase angles based on pos and speedDir
//----------------------------------------------------------------------------
void	setPhaseAngle(int16_t Yangle);
int16_t	getTransitionAngle(void);

#endif
