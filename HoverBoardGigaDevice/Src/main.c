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

#include "gd32f1x0.h"

#include "../Inc/setup.h"
#include "../Inc/defines.h"
#include "../Inc/config.h"
#include "../Inc/it.h"
#include "../Inc/bldc.h"
#include "../Inc/comms.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifdef MASTER
int32_t steer = 0; 												// global variable for steering. -1000 to 1000
int32_t speed = 0; 												// global variable for speed.    -1000 to 1000
FlagStatus activateWeakening = RESET;			// global variable for weakening
FlagStatus beepsBackwards = RESET;  			// global variable for beeps backwards
FlagStatus upperLEDSlave = RESET;					// global variable for slave upper LED
FlagStatus lowerLEDSlave = RESET;   			// global variable for slave lower LED
FlagStatus mosfetOutSlave = RESET;  			// global variable for slave mosfet out
			
extern uint8_t buzzerFreq;    						// global variable for the buzzer pitch. can be 1, 2, 3, 4, 5, 6, 7...
extern uint8_t buzzerPattern; 						// global variable for the buzzer pattern. can be 1, 2, 3, 4, 5, 6, 7...
			
extern float batteryVoltage; 							// global variable for battery voltage
extern float currentDC; 									// global variable for current dc
float currentDCSlave = 0;									// global variable for current dc from slave
uint8_t slaveError = 0;										// global variable for slave error
	
extern FlagStatus timedOut;								// Timeoutvariable set by timeout timer

uint32_t inactivity_timeout_counter = 0;	// Inactivity counter
uint32_t steerCounter = 0;								// Steer counter for setting update rate
steering_device_TX_buffer steerBuffer;		// Steer buffer for steering device

void ShowBatteryState(uint32_t pin);
void BeepsBackwards(FlagStatus beepsBackwards);
void ShutOff(void);
#endif

//----------------------------------------------------------------------------
// MAIN function
//----------------------------------------------------------------------------
int main (void)
{
#ifdef MASTER
	FlagStatus enable = RESET;
	FlagStatus enableSlave = RESET;
	FlagStatus chargeStateLowActive = SET;
	int8_t index = 8;
  int16_t pwmSlave = 0;
	int16_t pwmMaster = 0;
	int16_t weakSlave = 0;
	int16_t weakMaster = 0;
#endif
	
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
	USART_MasterSlave_init();
	
	// Init ADC
	ADC_init();
	
	// Init PWM
	PWM_init();
	
	// Device has 1,6 seconds to do all the initialization
	// afterwards watchdog will be fired
	fwdgt_counter_reload();
	
#ifdef MASTER
	// Init usart steer
	USART_Steer_init();
	
	// Startup-Sound
	for (; index >= 0; index--)
	{
    buzzerFreq = index;
    Delay(10);
  }
  buzzerFreq = 0;

	// Wait until button is pressed
	while (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN))
	{
		// Reload watchdog while button is pressed
		fwdgt_counter_reload();
	}
#endif
		
  while(1)
	{
#ifdef MASTER
		steerCounter++;	
		if ((steerCounter % 2) == 0)
		{
			steerBuffer.v_batt = batteryVoltage;
			steerBuffer.current_dc_Master = currentDC;
			steerBuffer.current_dc_Slave = currentDCSlave;
			steerBuffer.realSpeed = 0;
			steerBuffer.activateWeakening = activateWeakening;
			steerBuffer.beepsBackwards = beepsBackwards;
			steerBuffer.mosfetOutSlave = mosfetOutSlave;
			steerBuffer.lowerLEDSlave = lowerLEDSlave;
			steerBuffer.upperLEDSlave = upperLEDSlave;
			steerBuffer.mosfetOutMaster = gpio_input_bit_get(MOSFET_OUT_PORT, MOSFET_OUT_PIN);
			steerBuffer.lowerLEDMaster = gpio_input_bit_get(LOWER_LED_PORT, LOWER_LED_PIN);
			steerBuffer.upperLEDMaster = gpio_input_bit_get(UPPER_LED_PORT, UPPER_LED_PIN);
				
			// Send information to steering device and require steering data
			SendSteerDevice(steerBuffer);
		}
		
    // Mix steering and speed value for right and left speed
    pwmMaster = CLAMP(speed * SPEED_COEFFICIENT -  steer * STEER_COEFFICIENT, -1000, 1000);
    pwmSlave = CLAMP(speed * SPEED_COEFFICIENT +  steer * STEER_COEFFICIENT, -1000, 1000);

    #ifdef ADDITIONAL_CODE
    ADDITIONAL_CODE;
    #endif
		
		// Read charge state
		chargeStateLowActive = gpio_input_bit_get(CHARGE_STATE_PORT, CHARGE_STATE_PIN);
		
		// Enable is depending on charger is connected or not
		enable = chargeStateLowActive;
		
		// Enable channel output
		SetEnable(enable);

		// Decide if slave will be enabled
		enableSlave = (enable == SET && timedOut == RESET) ? SET : RESET;
		
    // Set output
		SetPWM(pwmMaster, weakMaster);
		SendPWMSlave(pwmSlave, weakSlave, enableSlave, RESET, chargeStateLowActive == 0 ? RESET : SET, RESET, chargeStateLowActive == 0 ? SET : RESET, upperLEDSlave, lowerLEDSlave, mosfetOutSlave);
		
		// Show green battery symbol when battery level FULL is reached
    if (batteryVoltage > BAT_FULL)
		{
			// Show green battery light
			ShowBatteryState(LED_GREEN);
			
			// Beeps backwards
			BeepsBackwards(beepsBackwards);
		}
		// Show orange battery symbol when battery level HALFFULL is reached
    else if (batteryVoltage > BAT_HALFFULL && batteryVoltage < BAT_FULL)
		{
			// Show orange battery light
			ShowBatteryState(LED_ORANGE);
			
			// Beeps backwards
			BeepsBackwards(beepsBackwards);
		}
		// Show red battery symbol when battery level BAT_LOW_LVL1 is reached
    else if (batteryVoltage > BAT_LOW_LVL1 && batteryVoltage < BAT_HALFFULL)
		{
			// Show red battery light
			ShowBatteryState(LED_RED);
			
			// Beeps backwards
			BeepsBackwards(beepsBackwards);
		}
		// Make silent sound when battery level BAT_LOW_LVL2 is reached
    else if (batteryVoltage > BAT_LOW_LVL2 && batteryVoltage < BAT_LOW_LVL1)
		{
			// Show red battery light
			ShowBatteryState(LED_RED);
			
      buzzerFreq = 5;
      buzzerPattern = 8;
    }
		// Make even more sound when battery level BAT_LOW_DEAD is reached
		else if  (batteryVoltage > BAT_LOW_DEAD && batteryVoltage < BAT_LOW_LVL2)
		{
			// Show red battery light
			ShowBatteryState(LED_RED);
			
      buzzerFreq = 5;
      buzzerPattern = 1;
    }
		// Shut device off, when battery is dead or charger is connected (chargeStateLowActive = 0)
		else if (batteryVoltage < BAT_LOW_DEAD)
		{
      ShutOff();
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
		
		// Calculate inactivity timeout
    if (ABS(pwmMaster) > 50 || ABS(pwmSlave) > 50 || !chargeStateLowActive)
		{
      inactivity_timeout_counter = 0;
    }
		else
		{
      inactivity_timeout_counter++;
    }
		
		// Shut off device after INACTIVITY_TIMEOUT in minutes
    if (inactivity_timeout_counter > (INACTIVITY_TIMEOUT * 60 * 1000) / (DELAY_IN_MAIN_LOOP + 1))
		{ 
      ShutOff();
    }
#endif	

		Delay(DELAY_IN_MAIN_LOOP);
		
		// Reload watchdog (watchdog fires after 1,6 seconds)
		fwdgt_counter_reload();
  }
}

#ifdef MASTER
//----------------------------------------------------------------------------
// Turns the device off
//----------------------------------------------------------------------------
void ShutOff(void)
{
	int index = 0;

	buzzerPattern = 0;
	for (; index < 8; index++)
	{
		buzzerFreq = index;
		Delay(10);
	}
	buzzerFreq = 0;
	
	// Send shut off command to slave
	SendPWMSlave(0, 0, RESET, SET, RESET, RESET, RESET, RESET, RESET, RESET);
	
	// Disable usart
	usart_deinit(USART_MASTERSLAVE);
	
	// Set pwm and enable to off
	SetEnable(RESET);
	SetPWM(0, 0);
	
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

//----------------------------------------------------------------------------
// Beeps while driving backwards
//----------------------------------------------------------------------------
void BeepsBackwards(FlagStatus beepsBackwards)
{
	// If the speed is less than -50, beep while driving backwards
	if (beepsBackwards == SET && speed < -50)
	{
		buzzerFreq = 5;
    buzzerPattern = 4;
	}
	else
	{
		buzzerFreq = 0;
		buzzerPattern = 0;
	}
}
#endif
