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

#define ARM_MATH_CM3

#include "gd32f1x0.h"

#include "../Inc/setup.h"
#include "../Inc/defines.h"
#include "../Inc/config.h"
#include "../Inc/it.h"
#include "../Inc/bldc.h"
#include "../Inc/commsHUGS.h"
#include "../Inc/commsSteering.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>     

#ifdef MASTER
int32_t speed = 0; 												// global variable for speed.    -1000 to 1000
FlagStatus activateWeakening = RESET;			// global variable for weakening
			
extern float batteryVoltage; 							// global variable for battery voltage
extern float currentDC; 									// global variable for current dc
extern float realSpeed; 									// global variable for real Speed
	
extern FlagStatus timedOut;								// Timeoutvariable set by timeout timer

extern bool			HUGS_Enabled;							// Set by HUGS communications
extern bool			HUGS_ESTOP;
extern uint16_t	HUGS_WatchDog;

extern uint8_t buzzerFreq;    						// global variable for the buzzer pitch. can be 1, 2, 3, 4, 5, 6, 7...
extern uint8_t buzzerPattern; 						// global variable for the buzzer pattern. can be 1, 2, 3, 4, 5, 6, 7...


uint16_t command_timeout_counter = 0;	    // motor safety counter ms From last command
uint32_t inactivity_timeout_counter = 0;	// Inactivity counter ms From last command

void ShowBatteryState(uint32_t pin);
void ShutOff(void);
#endif


//----------------------------------------------------------------------------
// MAIN function
//----------------------------------------------------------------------------
int main (void)
{
	FlagStatus chargeStateLowActive = SET;
	
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

	// Startup-Sound
	buzzerFreq = 7;
  Delay(100);
	fwdgt_counter_reload();
  buzzerFreq = 0;

	// Wait until button is pressed
	while (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN))
	{
		// Reload watchdog while button is pressed
		fwdgt_counter_reload();
	}

  while(1)
	{


		// Read charge state
		chargeStateLowActive = gpio_input_bit_get(CHARGE_STATE_PORT, CHARGE_STATE_PIN);
		
		// Show green battery symbol when battery level BAT_LOW_LVL1 is reached
    if (batteryVoltage > BAT_LOW_LVL1)
		{
			// Show green battery light
			ShowBatteryState(LED_GREEN);
		}
		// Make silent sound and show orange battery symbol when battery level BAT_LOW_LVL2 is reached
    else if (batteryVoltage > BAT_LOW_LVL2 && batteryVoltage < BAT_LOW_LVL1)
		{
			// Show orange battery light
			ShowBatteryState(LED_ORANGE);
    }
		// Make even more sound and show red battery symbol when battery level BAT_LOW_DEAD is reached
		else if  (batteryVoltage > BAT_LOW_DEAD && batteryVoltage < BAT_LOW_LVL2)
		{
			// Show red battery light
			ShowBatteryState(LED_RED);
    }
		else
		{
			ShutOff();
    }

		// Shut device off when button is pressed
		if (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN))
		{
      while (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN)) {}
			ShutOff();
    }
		
		// Shut off device after INACTIVITY_TIMEOUT in minutes
    if (inactivity_timeout_counter++ > (INACTIVITY_TIMEOUT * 60 * 1000) / (DELAY_IN_MAIN_LOOP + 1))
		{ 
      ShutOff();
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
	// Disable usart
	usart_deinit(USART_HUGS);
	
	// Set pwm and enable to off
	SetEnable(RESET);
	
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

