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

#ifndef COMMSHUGS_H
#define COMMSHUGS_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

#define	 MAX_SPEED	5000

typedef enum {NOP = 0, RSP, RES, ENA, DIS, POW, SPE, ABS, REL, DOG, MOD, DSPE=0x86, XXX = 0xFF} CMD_ID;
typedef enum {NOR = 0, SMOT, SPOW, SSPE, SPOS, SVOL, SAMP, SDOG, SMOD, SFPI, STOP = 0xFF} RSP_ID;

//----------------------------------------------------------------------------
// Update USART master slave input
//----------------------------------------------------------------------------
void UpdateUSARTHUGSInput(void);

//----------------------------------------------------------------------------
// Send a command across to the other controller.
//----------------------------------------------------------------------------
void SendHUGSCmd(CMD_ID SlaveCmd, int16_t value);

//----------------------------------------------------------------------------
// Send reply frame via USART
//----------------------------------------------------------------------------
void SendHUGSReply(void);

//----------------------------------------------------------------------------
// Signal an ESTOP power down
//----------------------------------------------------------------------------
void SetESTOP(void);


#endif
