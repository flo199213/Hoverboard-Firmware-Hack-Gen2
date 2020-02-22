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

//----------------------------------------------------------------------------
// Update USART master slave input
//----------------------------------------------------------------------------
void UpdateUSARTMasterSlaveInput(void);

#ifdef MASTER
//----------------------------------------------------------------------------
// Send slave frame via USART
//----------------------------------------------------------------------------
void SendSlave(int16_t pwmSlave, FlagStatus enable, FlagStatus shutoff, FlagStatus chargeState, uint8_t identifier, int16_t value);
#endif
#ifdef SLAVE
//----------------------------------------------------------------------------
// Send master frame via USART
//----------------------------------------------------------------------------
void SendMaster(FlagStatus upperLEDMaster, FlagStatus lowerLEDMaster, FlagStatus mosfetOutMaster, FlagStatus beepsBackwards);

//----------------------------------------------------------------------------
// Returns current value sent by master
//----------------------------------------------------------------------------
int16_t GetCurrentDCMaster(void);

//----------------------------------------------------------------------------
// Returns battery value sent by master
//----------------------------------------------------------------------------
int16_t GetBatteryMaster(void);

//----------------------------------------------------------------------------
// Returns realspeed value sent by master
//----------------------------------------------------------------------------
int16_t GetRealSpeedMaster(void);

//----------------------------------------------------------------------------
// Sets upper LED value which will be send to master
//----------------------------------------------------------------------------
void SetUpperLEDMaster(FlagStatus value);

//----------------------------------------------------------------------------
// Returns upper LED value sent by master
//----------------------------------------------------------------------------
FlagStatus GetUpperLEDMaster(void);

//----------------------------------------------------------------------------
// Sets lower LED value which will be send to master
//----------------------------------------------------------------------------
void SetLowerLEDMaster(FlagStatus value);

//----------------------------------------------------------------------------
// Returns lower LED value sent by master
//----------------------------------------------------------------------------
FlagStatus GetLowerLEDMaster(void);
	
//----------------------------------------------------------------------------
// Sets mosfetOut value which will be send to master
//----------------------------------------------------------------------------
void SetMosfetOutMaster(FlagStatus value);

//----------------------------------------------------------------------------
// Returns MosfetOut value sent by master
//----------------------------------------------------------------------------
FlagStatus GetMosfetOutMaster(void);

//----------------------------------------------------------------------------
// Sets beepsBackwards value which will be send to master
//----------------------------------------------------------------------------
void SetBeepsBackwardsMaster(FlagStatus value);

//----------------------------------------------------------------------------
// Returns beepsBackwardsMaster value sent by master
//----------------------------------------------------------------------------
FlagStatus GetBeepsBackwardsMaster(void);
#endif

#endif
