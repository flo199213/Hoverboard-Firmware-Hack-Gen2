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
*
* Overall Project Revision History
* Rev		Date			Description
* 2.0		5/8/2020	Switch to HUGS 2.0  (Little Endian)
*									Change speeds over to mm/s
* 2.1		5/14/2020	Added Motion Response.
*/

#define ARM_MATH_CM3

#include "gd32f1x0.h"

#include "../Inc/setup.h"
#include "../Inc/defines.h"
#include "../Inc/config.h"
#include "../Inc/it.h"
#include "../Inc/bldc.h"
#include "../Inc/comms.h"
#include "../Inc/commsHUGS.h"
#include "../Inc/commsSteering.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>     

FlagStatus activateWeakening = RESET;			// global variable for weakening
			
extern uint16_t batteryVoltagemV;					// global variable for battery voltage
	
extern FlagStatus timedOut;								// Timeoutvariable set by timeout timer

extern bool			HUGS_ESTOP;

extern uint8_t buzzerFreq;    						// global variable for the buzzer pitch.   can be 1, 2, 3, 4, 5, 6, 7...
extern uint8_t buzzerPattern; 						// global variable for the buzzer pattern. can be 1, 2, 3, 4, 5, 6, 7...

uint32_t inactivityTimer = 0;			  			// Inactivity counter ms From last command

void ShowBatteryState(uint32_t pin);
void ShutOff(void);

//----------------------------------------------------------------------------
// MAIN function
//----------------------------------------------------------------------------
int main (void)
{
	//SystemClock_Config();
  SystemCoreClockUpdate();
  SysTick_Config(SystemCoreClock / 100);
	
	// Init watchdog
	if (Watchdog_init() == ERROR)
	{
		// If an error accours with watchdog initialization do not start device
		while(1);
	}
	
	// Init Interrupts
	Interrupt_init();
	
	// Init timeout timer
	TimeoutTimer_init();
	
	// Init GPIOs
	GPIO_init();
	
	// Activate self hold direct after GPIO-init
	gpio_bit_write(SELF_HOLD_PORT, SELF_HOLD_PIN, SET);

	// Init usart master slave
	USART_HUGS_init();
	
	// Init ADC
	ADC_init();
	
	// Init PWM
	PWM_init();
	
	// Device has 1,6 seconds to do all the initialization
	// afterwards watchdog will be fired
	fwdgt_counter_reload();

	// Init usart steer/bluetooth
	USART_Steer_COM_init();

	// Startup-Sound.  This plays for 1 second
	buzzerFreq = 7;
  Delay(1000);
	fwdgt_counter_reload();
  buzzerFreq = 0;

	// Wait until button is pressed ????
	while (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN))
	{
		// Reload watchdog while button is pressed
		fwdgt_counter_reload();
	}
	
  while(1)
	{
		// Shut device if ESTOP requested or when button is pressed
		if (HUGS_ESTOP  || gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN))
		{
			ShutOff();
    }

		// Shut off device after INACTIVITY_TIMEOUT in minutes
    if (inactivityTimer++ > INACTIVITY_COUNTER)
		{ 
      ShutOff();
    } else if (inactivityTimer > INACTIVITY_WARNING) { 
			buzzerFreq = 8;
      buzzerPattern = 8;
    } else {
		
			// Show green battery symbol when battery level BAT_LOW_LVL1 is reached
			if (batteryVoltagemV > BAT_LOW_LVL1_MV)
			{
				// Show green battery light
				ShowBatteryState(LED_GREEN);
				buzzerFreq = 0;
			}
			// Make silent sound and show orange battery symbol when battery level BAT_LOW_LVL2 is reached
			else if (batteryVoltagemV > BAT_LOW_LVL2_MV && batteryVoltagemV < BAT_LOW_LVL1_MV)
			{
				// Show orange battery light
				ShowBatteryState(LED_ORANGE);
				buzzerFreq = 5;
				buzzerPattern = 8;
			}
			// Make even more sound and show red battery symbol when battery level BAT_LOW_DEAD is reached
			else if  (batteryVoltagemV > BAT_LOW_DEAD_MV && batteryVoltagemV < BAT_LOW_LVL2_MV)
			{
				// Show red battery light
				ShowBatteryState(LED_RED);
				buzzerFreq = 5;
				buzzerPattern = 1;
			}
			else
			{
				ShutOff();
			}
		}


		Delay(DELAY_IN_MAIN_LOOP);
		
		// Reload watchdog (watchdog fires after 1,6 seconds)
		fwdgt_counter_reload();
  }
}

//----------------------------------------------------------------------------
// Turns the device off
//----------------------------------------------------------------------------
void ShutOff(void)
{
	int index = 0;
	buzzerPattern = 0;

	// Reload watchdog (watchdog fires after 1,6 seconds)
	fwdgt_counter_reload();
	
	// Ensure that drive is off and estop status set.
	SetPWM(0);
	HUGS_ESTOP   = TRUE;	
	SetEnable(RESET);
	SendHUGSReply();			// Transfer ESTOP to Controller
	SendHUGSCmd(XXX, 0);	// Tell possible slave to stop as well.
	

	// Play shutdown sound
	for (index = 0; index < 8; index++)
	{
		buzzerFreq = index;
		Delay(100);
	}
	buzzerFreq = 0;

	// Disable usart
	usart_deinit(USART_HUGS);
	
	// Turn off power
	gpio_bit_write(SELF_HOLD_PORT, SELF_HOLD_PIN, RESET);
	while(1)
	{
		// Reload watchdog until device is off
		fwdgt_counter_reload();
	}
}

//----------------------------------------------------------------------------
// Shows the battery state on the LEDs
//----------------------------------------------------------------------------
void ShowBatteryState(uint32_t pin)
{
	gpio_bit_write(LED_GREEN_PORT, LED_GREEN, pin == LED_GREEN ? SET : RESET);
	gpio_bit_write(LED_ORANGE_PORT, LED_ORANGE, pin == LED_ORANGE ? SET : RESET);
	gpio_bit_write(LED_RED_PORT, LED_RED, pin == LED_RED ? SET : RESET);
}


void	resetInactivityTimer(void) {
	inactivityTimer = 0;
}
