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
* could find ab better way through the code.
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

#ifndef SETUP_H
#define SETUP_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

#define USART_STEER_RX_BUFFERSIZE 1
#define USART_STEER_DATA_RX_ADDRESS ((uint32_t)0x40013824)
#define USART_MASTERSLAVE_RX_BUFFERSIZE 1
#define USART_MASTERSLAVE_DATA_RX_ADDRESS ((uint32_t)0x40004424)

#ifdef MASTER
#define I2C0_OWN_ADDRESS7 0x40
#define STEERINGDEVICE_SLAVE_ADDRESS7 0x82

//#define I2C_RXDATA_ADDRESS I2C_DATA(I2C0)
//#define I2C_TXDATA_ADDRESS I2C_DATA(I2C0)

//#define I2C_STEER_RX_BUFFERSIZE 1
//#define I2C_STEER_TX_BUFFERSIZE 1
#endif

//----------------------------------------------------------------------------
// Initializes the interrupts
//----------------------------------------------------------------------------
void Interrupt_init(void);

//----------------------------------------------------------------------------
// Initializes the watchdog
//----------------------------------------------------------------------------
ErrStatus Watchdog_init(void);

//----------------------------------------------------------------------------
// Initializes the timeout timer
//----------------------------------------------------------------------------
void TimeoutTimer_init(void);

//----------------------------------------------------------------------------
// Initializes the GPIOs
//----------------------------------------------------------------------------
void GPIO_init(void);

//----------------------------------------------------------------------------
// Initializes the PWM
//----------------------------------------------------------------------------
void PWM_init(void);

//----------------------------------------------------------------------------
// Initializes the ADC
//----------------------------------------------------------------------------
void ADC_init(void);

//----------------------------------------------------------------------------
// Initializes the usart master slave
//----------------------------------------------------------------------------
void USART_MasterSlave_init(void);

#ifdef MASTER
//----------------------------------------------------------------------------
// Initializes the steer usart
//----------------------------------------------------------------------------
void USART_Steer_init(void);
#endif

#endif
