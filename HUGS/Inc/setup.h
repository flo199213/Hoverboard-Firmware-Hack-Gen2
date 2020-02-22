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

#ifndef SETUP_H
#define SETUP_H

#include "gd32f1x0.h"
#include "../Inc/config.h"


#define USART_HUGS_RX_BUFFERSIZE 1
#define USART_HUGS_DATA_RX_ADDRESS ((uint32_t)0x40004424)

#define USART_STEER_COM_RX_BUFFERSIZE 1
#define USART_STEER_COM_DATA_RX_ADDRESS ((uint32_t)0x40013824)

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
void USART_HUGS_init(void);

//----------------------------------------------------------------------------
// Initializes the steer/bluetooth usart
//----------------------------------------------------------------------------
void USART_Steer_COM_init(void);

#endif
