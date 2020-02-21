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

//----------------------------------------------------------------------------
// Set motor enable
//----------------------------------------------------------------------------
void SetEnable(FlagStatus setEnable);

//----------------------------------------------------------------------------
// Set pwm -1000 to 1000
//----------------------------------------------------------------------------
void SetPWM(int16_t setPwm);

//----------------------------------------------------------------------------
// Calculation-Routine for BLDC => calculates with 16kHz
//----------------------------------------------------------------------------
void CalculateBLDC(void);

#endif
