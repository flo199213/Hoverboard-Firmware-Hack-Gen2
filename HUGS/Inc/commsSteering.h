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

#ifndef COMMSSTEERING_H
#define COMMSSTEERING_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

// Only master communicates with steering device
#ifdef MASTER
//----------------------------------------------------------------------------
// Update USART steer input
//----------------------------------------------------------------------------
void UpdateUSARTSteerInput(void);

//----------------------------------------------------------------------------
// Send frame to steer device
//----------------------------------------------------------------------------
void SendSteerDevice(void);
#endif

#endif
