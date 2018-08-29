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

#ifndef COMMSMASTERSLAVE_H
#define COMMSMASTERSLAVE_H

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
