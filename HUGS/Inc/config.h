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

#ifndef CONFIG_H
#define CONFIG_H

#include "gd32f1x0.h"

// ################################################################################

#define MASTER										  	// Select if firmware is for master or slave board
//#define SLAVE 												// Select if firmware is for master or slave board

// ################################################################################

#define PWM_FREQ         		16000     // PWM frequency in Hz
#define DEAD_TIME        		60        // PWM deadtime (60 = 1µs, measured by oscilloscope)

#define DC_CUR_LIMIT     		15        // Motor DC current limit in amps
#define DC_CUR_LIMIT_MA  		15000     // Motor DC current limit in milli Amps

// ################################################################################

#define DELAY_IN_MAIN_LOOP 	5         // Delay in ms
#define TIMEOUT_MS          2000      // Time in milliseconds without steering commands before pwm emergency off
#define INACTIVITY_TIMEOUT 	8        	// Minutes of not driving until poweroff (not very precise)

// ################################################################################

#define BAT_LOW_LVL1     35.0       // Gently beeps, show green battery symbol above this Level.
#define BAT_LOW_LVL2     33.0       // Battery almost empty, show orange battery symbol above this Level. Charge now! 
#define BAT_LOW_DEAD     31.0       // Undervoltage lockout, show red battery symbol above this Level.

#define BAT_LOW_LVL1_MV  35000      // Gently beeps, show green battery symbol above this Level.
#define BAT_LOW_LVL2_MV  33000      // Battery almost empty, show orange battery symbol above this Level. Charge now! 
#define BAT_LOW_DEAD_MV  31000      // Undervoltage lockout, show red battery symbol above this Level.

// ################################################################################

// ###### ARMCHAIR ######
#define PWM_FILTER_SHIFT  10   			// Low-pass filter for pwm, rank k=12
#define SPEED_FILTER_SHIFT 7 				// Low-pass filter for phase period, rank k=10

#endif
