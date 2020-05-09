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

#include "gd32f1x0.h"
#include "../Inc/setup.h"
#include "../Inc/defines.h"
#include "../Inc/config.h"

#include "../Inc/bldc.h"

// Internal constants
const int16_t pwm_res = 72000000 / 2 / PWM_FREQ; // = 2000

const int32_t WHEEL_PERIMETER    = 530 ;  // mm
const int32_t SPEED_TICKS_FACTOR = 188444 ;  // Divide factor by speed to get ticks per cycle, or visa versa.
const int16_t MIN_SPEED          = 10 ;      // min usable speed in mm/S
const int16_t MAX_PHASE_PERIOD   = SPEED_TICKS_FACTOR / MIN_SPEED ;   // one phase count @ MIN_SPEED
const uint16_t STEPPER_LIMIT     = 500 ;     // Switch to stepper mode if requested speed < 500mm/S 
const int16_t STEPPER_POWER      = 250 ;     // Power to run stepper motor

#define KF 2 / 11
#define KP 3 / 5

int16_t speedError;

uint16_t batteryVoltagemV = 40000;
uint16_t currentDCmA      = 0;
int16_t realSpeedmmPS     = 0;
int32_t cycles            = 0;
int16_t speedCounter 			= 0;
int16_t phasePeriod 			= 0;
int8_t  stepDir						= 0;

bool		stepperMode				= FALSE;
int32_t stepperPeriod     = 0;
int32_t stepperTicks      = 0;
int8_t  stepperPos 		    = 0;
int8_t  stepperDir				= 0;

// Timeoutvariable set by timeout timer
extern FlagStatus timedOut;

// Variables to be set from the main routine
int16_t 		bldcInputPwm 		= 0;
FlagStatus 	bldcEnable 			= RESET;
int16_t 		speedSetpoint   = 0;
bool				constantSpeed		= FALSE;

// ADC buffer to be filled by DMA
adc_buf_t adc_buffer;

// Internal calculation variables
uint8_t hall_a;
uint8_t hall_b;
uint8_t hall_c;
uint8_t hall;
uint8_t pos;
uint8_t lastPos;

FlagStatus buzzerToggle = RESET;
uint8_t  buzzerFreq = 0;
uint8_t  buzzerPattern = 0;
uint16_t buzzerTimer = 0;
int16_t  offsetcount = 0;
int16_t  offsetdc = 2000;

// Low Pass Filter Regs
int32_t phasePeriodFilterReg = 0;
int16_t filteredPhasePeriod = 0;

int32_t PWMFilterReg   = 0;
int16_t bldcFilteredPwm = 0;



//----------------------------------------------------------------------------
// Commutation table
//----------------------------------------------------------------------------
const uint8_t hall_to_pos[8] =
{
	// annotation: for example SA=0 means hall sensor pulls SA down to Ground
  0, // hall position [-] - No function (access from 1-6) 
  3, // hall position [1] (SA=1, SB=0, SC=0) -> PWM-position 3
  5, // hall position [2] (SA=0, SB=1, SC=0) -> PWM-position 5
  4, // hall position [3] (SA=1, SB=1, SC=0) -> PWM-position 4
  1, // hall position [4] (SA=0, SB=0, SC=1) -> PWM-position 1
  2, // hall position [5] (SA=1, SB=0, SC=1) -> PWM-position 2
  6, // hall position [6] (SA=0, SB=1, SC=1) -> PWM-position 6
  0, // hall position [-] - No function (access from 1-6) 
};

//----------------------------------------------------------------------------
// Block PWM calculation based on position
//----------------------------------------------------------------------------
__INLINE void blockPWM(int pwm, int pwmPos, int *y, int *b, int *g)
{
  switch(pwmPos)
	{
    case 1:
      *b = pwm;
      *y = 0;
      *g = -pwm;
      break;
    case 2:
      *b = pwm;
      *y = -pwm;
      *g = 0;
      break;
    case 3:
      *b = 0;
      *y = -pwm;
      *g = pwm;
      break;
    case 4:
      *b = -pwm;
      *y = 0;
      *g = pwm;
      break;
    case 5:
      *b = -pwm;
      *y = pwm;
      *g = 0;
      break;
    case 6:
      *b = 0;
      *y = pwm;
      *g = -pwm;
      break;
    default:
      *b = 0;
      *y = 0;
      *g = 0;
  }
}

//----------------------------------------------------------------------------
// Set motor enable
//----------------------------------------------------------------------------
void SetEnable(FlagStatus setEnable)
{
	if (!bldcEnable || !setEnable) {
		stepperMode   = FALSE;
	}
	bldcEnable = setEnable;
}

//----------------------------------------------------------------------------
// Set speed -100 to 100
//----------------------------------------------------------------------------
void SetSpeed(int16_t speed)
{
	constantSpeed = TRUE;
	speedSetpoint = CLAMP(speed, -5000, 5000);
	
  if (abs16(speedSetpoint) < STEPPER_LIMIT) {  // Sorry about dumb syntax.  Compiler threw warning.
		
		if (stepperMode == FALSE) {
			stepperPos = pos;
		}
		
		stepperMode = TRUE; 							 // Turn on stepper for slow speeds
		stepperPeriod = SPEED_TICKS_FACTOR / abs16(speedSetpoint) ;
		
	} else {
		stepperMode = FALSE;
	}
}

//----------------------------------------------------------------------------
// Set power -100 to 100
//----------------------------------------------------------------------------
void SetPower(int16_t power)
{
	constantSpeed = FALSE;
	stepperMode   = FALSE;
	speedSetpoint = 0;
	
	// stepperTicks  = 0;
	SetPWM(power);
}

//----------------------------------------------------------------------------
// Set pwm -1000 to 1000
//----------------------------------------------------------------------------
void SetPWM(int16_t setPwm)
{
	bldcInputPwm = CLAMP(setPwm, -1000, 1000);
}


//----------------------------------------------------------------------------
// Get pwm -1000 to 1000
//----------------------------------------------------------------------------
int16_t GetPWM()
{
	return bldcFilteredPwm;
}

//----------------------------------------------------------------------------
// Get Revs Per Second * 100 -1000 to 1000
//----------------------------------------------------------------------------
int16_t GetSpeed()
{
	if (filteredPhasePeriod == MAX_PHASE_PERIOD ) {
		realSpeedmmPS = 0 ;
	}
	else{
		realSpeedmmPS = (SPEED_TICKS_FACTOR / filteredPhasePeriod) * stepDir ;
	}
	
	return realSpeedmmPS;
}

//----------------------------------------------------------------------------
// Calculation-Routine for BLDC => calculates with 32kHz  (was 16 kHz)
//----------------------------------------------------------------------------
void CalculateBLDC(void)
{
	int y = 0;     // yellow = phase A
	int b = 0;     // blue   = phase B
	int g = 0;     // green  = phase C
	int8_t stepDif;
	
	// Create square wave for buzzer
	buzzerTimer++;
	if (buzzerFreq != 0 && (buzzerTimer / 5000) % (buzzerPattern + 1) == 0){
		if (buzzerTimer % buzzerFreq == 0){
			buzzerToggle = buzzerToggle == RESET ? SET : RESET; // toggle variable
			gpio_bit_write(BUZZER_PORT, BUZZER_PIN, buzzerToggle);
		}
	} else {
		gpio_bit_write(BUZZER_PORT, BUZZER_PIN, RESET);
	}

	
	// Calibrate ADC offsets for the first 1000 cycles
  if (offsetcount < 1000) {  
    offsetcount++;
    offsetdc = (adc_buffer.current_dc + offsetdc) / 2;
    return;
  }
	
	// Calculate battery voltage & current every 100 cycles
  if (buzzerTimer % 100 == 0) {
		uint16_t tempV = (uint16_t)(((uint32_t)adc_buffer.v_batt * ADC_BATTERY_MICRO_VOLT) / 1000);
 		uint16_t tempI = (uint16_t)((ABS((adc_buffer.current_dc - offsetdc) * MOTOR_AMP_CONV_DC_MICRO_AMP)) / 1000);

		batteryVoltagemV 	+= ((tempV - batteryVoltagemV) / 100);
		currentDCmA 			+= ((tempI - currentDCmA) / 100);
  }
	

  // Disable PWM when current limit is reached (current chopping), enable is not set or timeout is reached
	if (currentDCmA > DC_CUR_LIMIT_MA || bldcEnable == RESET || timedOut == SET) {
		timer_automatic_output_disable(TIMER_BLDC);		
  } else {
		timer_automatic_output_enable(TIMER_BLDC);
  }
	
  // Read hall sensors
	hall_a = gpio_input_bit_get(HALL_A_PORT, HALL_A_PIN);
  hall_b = gpio_input_bit_get(HALL_B_PORT, HALL_B_PIN);
	hall_c = gpio_input_bit_get(HALL_C_PORT, HALL_C_PIN);
  
	// Determine current position based on hall sensors
  hall = (hall_a * 1) + (hall_b * 2) + (hall_c * 4);
  pos = hall_to_pos[hall];

	
	// Are we switching steps?  If so, time to calculate phase period 
	if (pos != lastPos) {
		phasePeriod = speedCounter;
		speedCounter = 0;
		
		// Check direction of rotation
		stepDif = pos - lastPos;
		if ((stepDif == 1) ||  (stepDif == -5))
			stepDir = -1;
		else if ((stepDif == -1) || (stepDif == 5))
			stepDir = +1;
	}
	
	// Calculate low-pass filter for phase Period
	phasePeriodFilterReg = phasePeriodFilterReg - (phasePeriodFilterReg >> PHASE_PERIOD_FILTER_SHIFT) + phasePeriod;
	filteredPhasePeriod = phasePeriodFilterReg >> PHASE_PERIOD_FILTER_SHIFT;
	
	// Determine desired PWM based on operational mode
	if (constantSpeed) {
		if (stepperMode) {
			stepperTicks++;
			
			// Determine stepper power and direction
			if (speedSetpoint > MIN_SPEED) {
				stepperDir = -1;
				SetPWM (STEPPER_POWER) ;
			} else if  (speedSetpoint < -MIN_SPEED) {
				SetPWM (-STEPPER_POWER) ;
				stepperDir = 1;
			} else {
				SetPWM (0) ;
				stepperDir = 0;
			}
		} else {
			// RUN P(id)F to set power
			
			// determine speed error and set power level
			speedError = (speedSetpoint - realSpeedmmPS);
			
			// Generate output  F = 1/5.4  (approx 2/11)  P = 3/5
			SetPWM ((speedSetpoint * KF) + (speedError * KP)) ;
		}
	}
	
	// Calculate low-pass filter for pwm value
	PWMFilterReg = PWMFilterReg - (PWMFilterReg >> PWM_FILTER_SHIFT) + bldcInputPwm;
	bldcFilteredPwm = PWMFilterReg >> PWM_FILTER_SHIFT;
	
	// Free run the wheel, or step it based on the mode
	if (stepperMode) {
    if (stepperTicks >= 1000) {
			// Update PWM channels based on position y(ellow), b(lue), g(reen)
			stepperPos += stepperDir ;
			if (stepperPos == 0)
				stepperPos = 6;
			else if (stepperPos == 7)
				stepperPos = 1;
			
			blockPWM(bldcFilteredPwm, stepperPos, &y, &b, &g);
			stepperTicks = 0;
		}
	} else {
		// Update PWM channels based on position y(ellow), b(lue), g(reen)
		blockPWM(bldcFilteredPwm, pos, &y, &b, &g);
	}
		
	// Set PWM output (pwm_res/2 is the mean value, setvalue has to be between 10 and pwm_res-10)
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, CLAMP(g + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, CLAMP(b + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, CLAMP(y + pwm_res / 2, 10, pwm_res-10));
	
	// Increments with 31.25 us
	if(speedCounter < MAX_PHASE_PERIOD) { // No speed less than MIN_SPEED
		speedCounter += 1;
	} else {
		phasePeriod = MAX_PHASE_PERIOD;  // 10 mm/S
		realSpeedmmPS = 0;
	}
	
	// Every time position reaches value 1, one cycle is performed (rising edge)
	// Integrate distance travelled.
	if (lastPos != 1 && pos == 1) {
		cycles += stepDir;
	}

	// Safe last position
	lastPos = pos;
}

int16_t	abs16 (int16_t value) {
	if (value < 0)
		value = -value;
	
	return value;
}
