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

#define FULL_PHASE      360
#define PHASE_Y_OFFSET	(0)
#define PHASE_B_OFFSET	(FULL_PHASE / 3)
#define PHASE_G_OFFSET	(FULL_PHASE * 2 / 3)
#define TRANSITION_ANGLE	233

// Internal constants
const int16_t pwm_res = 72000000 / 2 / PWM_FREQ; // = 2000
const int32_t WHEEL_PERIMETER    = 530 ;  // mm
const int32_t SPEED_TICKS_FACTOR = 188444 ;  // Divide factor by speed to get ticks per cycle, or visa versa.
const int32_t SINE_TICKS_FACTOR  = 3010   ;  // Divide factor by speed to get ticks per degree.
const int32_t MIN_SPEED          = 5 ;       // min usable speed in mm/S
const int32_t MAX_PHASE_PERIOD   = SPEED_TICKS_FACTOR / MIN_SPEED ;   // one phase count @ MIN_SPEED
const float MM_PER_CYCLE_FLOAT   = 5.888;	   //  (530 / 90)

//----------------------------------------------------------------------------
// Sinusoidal Power Commutation table (one degree increments. * 1000)
//----------------------------------------------------------------------------
const int16_t	sineTable[FULL_PHASE] = {0, 31, 63, 94, 117, 148, 180, 211, 242, 266, 297, 328, 359, 391, 414, 445, 477, 500, 531, 563, 586, 617, 648, 672, 703, 727, 758, 781, 813, 836, 859, 867, 875, 883, 891, 898, 906, 914, 922, 930, 938, 938, 945, 953, 953, 961, 969, 969, 977, 977, 977, 984, 984, 984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 984, 984, 984, 977, 977, 977, 969, 969, 961, 953, 953, 945, 938, 938, 930, 922, 914, 906, 898, 891, 883, 875, 867, 859, 867, 875, 883, 891, 898, 906, 914, 922, 930, 938, 938, 945, 953, 953, 961, 969, 969, 977, 977, 977, 984, 984, 984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 984, 984, 984, 977, 977, 977, 969, 969, 961, 953, 953, 945, 938, 938, 930, 922, 914, 906, 898, 891, 883, 875, 867, 859, 836, 813, 781, 758, 727, 703, 672, 648, 617, 586, 563, 531, 500, 477, 445, 414, 391, 359, 328, 297, 266, 242, 211, 180, 148, 117, 94, 63, 31, 0, -31, -63, -94, -117, -148, -180, -211, -242, -266, -297, -328, -359, -391, -414, -445, -477, -500, -531, -563, -586, -617, -648, -672, -703, -727, -758, -781, -813, -836, -859, -867, -875, -883, -891, -898, -906, -914, -922, -930, -938, -938, -945, -953, -953, -961, -969, -969, -977, -977, -977, -984, -984, -984, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -984, -984, -984, -977, -977, -977, -969, -969, -961, -953, -953, -945, -938, -938, -930, -922, -914, -906, -898, -891, -883, -875, -867, -859, -867, -875, -883, -891, -898, -906, -914, -922, -930, -938, -938, -945, -953, -953, -961, -969, -969, -977, -977, -977, -984, -984, -984, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -992, -984, -984, -984, -977, -977, -977, -969, -969, -961, -953, -953, -945, -938, -938, -930, -922, -914, -906, -898, -891, -883, -875, -867, -859, -836, -813, -781, -758, -727, -703, -672, -648, -617, -586, -563, -531, -500, -477, -445, -414, -391, -359, -328, -297, -266, -242, -211, -180, -148, -117, -94, -63, -31};

//----------------------------------------------------------------------------
// Commutation table Phase increments
//----------------------------------------------------------------------------
const uint8_t hall_to_pos[8] =
{
	// Note: Changed Comutation numbers 1-based to 0 based.
	// annotation: for example SA=0 means hall sensor pulls SA down to Ground
  6, // hall position [0] - No function (access from 0-5) 
  2, // hall position [1] (SA=1, SB=0, SC=0) -> PWM-position 2
  4, // hall position [2] (SA=0, SB=1, SC=0) -> PWM-position 4
  3, // hall position [3] (SA=1, SB=1, SC=0) -> PWM-position 3
  0, // hall position [4] (SA=0, SB=0, SC=1) -> PWM-position 0
  1, // hall position [5] (SA=1, SB=0, SC=1) -> PWM-position 1
  5, // hall position [6] (SA=0, SB=1, SC=1) -> PWM-position 5
  6, // hall position [7] - No function (access from 0-5) 
};

uint16_t batteryVoltagemV = 40000;
uint16_t currentDCmA      = 0;
int32_t cycles            = 0;
int32_t speedCounter 			= 0;
int32_t phasePeriod 			= 0;
int8_t  stepDir	 				  = 0;  // determined rotation direction
int8_t  speedDir					= 0;  // commanded rotation direction
int8_t  controlMode				= 0;  // 1,2 or 3
uint8_t speedMode					= DEFAULT_SPEED_MODE;      // Desired Closed Loop Speed Mode.  0 = PF, 1 = STEP, 2 = Dual 
uint8_t maxStepSpeed			= DEFAULT_MAX_STEP_SPEED;  // 

int16_t outF 							= 0;
int16_t outP 							= 0;
int16_t outI 							= 0;
	
int32_t speedError 				= 0;
int32_t errorIntegral			= 0;	// PID components scaled up by 15 bits

bool		stepperMode				= FALSE;
bool		phaseRestart			= FALSE;
int32_t stepperPeriod     = 0;
int32_t stepperTicks      = 0;

int16_t	step_y						= PHASE_Y_OFFSET;
int16_t	step_b						= PHASE_B_OFFSET;
int16_t	step_g						= PHASE_G_OFFSET;

int 		y 								= 0;     // yellow = phase A
int 		b 								= 0;     // blue   = phase B
int 		g 								= 0;     // green  = phase C

// Timeoutvariable set by timeout timer
extern FlagStatus timedOut;

// Variables to be set from the main routine
int16_t 		bldcInputPwm 		= 0;
FlagStatus 	bldcEnable 			= RESET;
int32_t 		speedSetpoint   = 0;
int32_t 		lastSpeedSetpoint   = 0;
bool				closedLoopSpeed	= FALSE;

// ADC buffer to be filled by DMA
adc_buf_t adc_buffer;

// Internal calculation variables
uint8_t pos;
uint8_t lastPos;

FlagStatus buzzerToggle = RESET;
uint8_t  buzzerFreq = 0;
uint8_t  buzzerPattern = 0;
uint16_t buzzerTimer = 0;
int16_t  offsetcount = 0;
int16_t  offsetdc = 2000;

// Low Pass Filter Regs
int32_t realSpeedFilterReg = 0;
int16_t realSpeedmmPS      = 0;

int32_t PWMFilterReg    = 0;
int16_t bldcFilteredPwm = 0;


//----------------------------------------------------------------------------
// Block PWM calculation based on position
//----------------------------------------------------------------------------
__INLINE void blockPWM(int pwm, int pwmPos, int *y, int *b, int *g)
{
	// Note:  These now cycle from 0 to 5 with positive applied PWM
  switch(pwmPos)
	{
    case 0:
      *y = 0;
      *b = -pwm;
      *g = pwm;
      break;
    case 1:
      *y = pwm;
      *b = -pwm;
      *g = 0;
      break;
    case 2:
      *y = pwm;
      *b = 0;
      *g = -pwm;
      break;
    case 3:
      *y = 0;
      *b = pwm;
      *g = -pwm;
      break;
    case 4:
      *y = -pwm;
      *b = pwm;
      *g = 0;
      break;
    case 5:
      *y = -pwm;
      *b = 0;
      *g = pwm;
      break;
    default:
      *y = 0;
      *b = 0;
      *g = 0;
  }
}

//----------------------------------------------------------------------------
// Set motor enable
//----------------------------------------------------------------------------
void SetEnable(FlagStatus setEnable)
{
	if (!bldcEnable || !setEnable) {
		closedLoopSpeed   = FALSE;
	}
	bldcEnable = setEnable;
}

//----------------------------------------------------------------------------
// Set speed -5000 to 5000
//----------------------------------------------------------------------------
void SetSpeed(int16_t speed)
{
	//  Keep driver alive
	if (abs16(speed) >= 10) {
		resetInactivityTimer();
	}

	closedLoopSpeed = TRUE;
	speedSetpoint = CLAMP(speed, -5000, 5000);
	
	if (speed > 0)
		speedDir = 1;
	else if (speed < 0) 
		speedDir = -1;
	else {
		speedDir = 0;
		controlMode = 0;
	}
	
	// Do we need stepper mode?
  if ( (speedMode == SPEED_MODE_PF) ||
       ((speedMode == SPEED_MODE_DUAL) && (abs16(speedSetpoint) > maxStepSpeed))
		 ) { 
		stepperMode = FALSE;
		phaseRestart = FALSE;
	} else {

		// do we need to defer switching to stepper to get synched?
		// we do this is we are currently in closed loop and are slowing down.
		// If we don't do this, the wheel jumps forward or back
		if (controlMode == 3) {
			phaseRestart = TRUE;
		} else {
			stepperMode = TRUE; 							 // Turn on stepper now
		}
		
		stepperPeriod = SINE_TICKS_FACTOR / abs16(speedSetpoint) ;
	}
}

//----------------------------------------------------------------------------
// Set power -1000 to 1000
//----------------------------------------------------------------------------
void SetPower(int16_t power)
{
	//  Keep driver alive
	if (abs16(power) > 5) {
		resetInactivityTimer();
	}
	closedLoopSpeed = FALSE;
	stepperMode   = FALSE;
	speedSetpoint = 0;
	
	if (power == 0)
		controlMode = 0;
	
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
	return bldcFilteredPwm ;
}

//----------------------------------------------------------------------------
// Get speed in mm/Sec
//----------------------------------------------------------------------------
int16_t GetSpeed()
{
	return realSpeedmmPS ;
}	

//----------------------------------------------------------------------------
// Get speed in mm/Sec
//----------------------------------------------------------------------------
void CalculateSpeed()
{
	int16_t speed;
	
	if (phasePeriod == MAX_PHASE_PERIOD ) {
		speed = 0 ;
	}
	else{
		speed = (SPEED_TICKS_FACTOR / phasePeriod) * stepDir ;
	}
	
	// Calculate low-pass filter for phase Period
	realSpeedFilterReg = realSpeedFilterReg - (realSpeedFilterReg >> SPEED_FILTER_SHIFT) + speed;
	realSpeedmmPS = realSpeedFilterReg >> SPEED_FILTER_SHIFT;
}

//----------------------------------------------------------------------------
// Get position in mm
//----------------------------------------------------------------------------
int32_t GetPosition(void) {
	return (int32_t)((float)cycles * MM_PER_CYCLE_FLOAT) ;
}

//----------------------------------------------------------------------------
// Calculation-Routine for BLDC => calculates with 32kHz  (was 16 kHz)
// Written so there are no function calls requiring stack protection.
//----------------------------------------------------------------------------
void CalculateBLDC(void)
{
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

	// Get current commutation position	
  pos = hallToPos();
	
	// Are we switching phase steps?  If so, time to calculate phase period 
	if (pos != lastPos) {
		phasePeriod = speedCounter;
		speedCounter = 0;
		
		// Check direction of rotation
		stepDif = pos - lastPos;
		if ((stepDif == 1) ||  (stepDif == -5))
			stepDir = 1;
		else if ((stepDif == -1) || (stepDif == 5))
			stepDir = -1;

		// Integrate steps to measure distance
		cycles += stepDir;
		
		// if we are waiting for a phase restart, load new angle.
		if (phaseRestart) {
			phaseRestart = FALSE;
			stepperMode = TRUE;
			
			setPhaseAngle(getTransitionAngle());
		}
	}
	
	// Accumulate counters for speed and phase duration
	// Increments with 31.25 us
	if(speedCounter < MAX_PHASE_PERIOD) { // No speed less than MIN_SPEED
		speedCounter += 1;
	} else {
		phasePeriod = MAX_PHASE_PERIOD;  // MIN_SPEED phase duration 
		realSpeedmmPS = 0;
	}
	
	// ======================================================================================	
	// Determine desired phase commutation and PWM based on operational mode
	// Three conditions are:  
	// 1) Open loop power
	// 2) Closed Loop Stepping (Slow Speed)
	// 3) Closed Loop Running (PIDF), (Fast Speed)
	// ======================================================================================	
	
	if (closedLoopSpeed) {

		if (speedSetpoint == 0){
			SetPWM(0) ;
		} else {
			
			// determine speed error and set power level
			SetPWM(runPID());
	
		}

		// Calculate low-pass filter for pwm value (we don't always need it. but best to keep running.)
		PWMFilterReg = PWMFilterReg - (PWMFilterReg >> PWM_FILTER_SHIFT) + bldcInputPwm;
		bldcFilteredPwm = PWMFilterReg >> PWM_FILTER_SHIFT;

		if (stepperMode) {
			// 2) Closed Loop Stepping (Slow Speed)
			controlMode = 2;
			stepperTicks++;
			
			// Are we moving to the next angle?
			if (stepperTicks >= stepperPeriod) {
				setPhaseAngle(step_y + speedDir);
				stepperTicks = 0;
			}
				
			// Get three PWM values from sine table 
			y = sineTable[step_y] >> 3;	
			b = sineTable[step_g] >> 3;	
			g = sineTable[step_b] >> 3;	
			
		} else {
			// 3) Closed Loop Running (PIDF), (Fast Speed)
			controlMode = 3;
		
			// Update PWM channels based on position y(ellow), b(lue), g(reen)
			blockPWM(bldcFilteredPwm, pos, &y, &b, &g);
		}
	} else {
		// 1) Open loop power, so just apply filtered PWM to correct phases
		controlMode = 1;

		// Calculate low-pass filter for pwm value
		PWMFilterReg = PWMFilterReg - (PWMFilterReg >> PWM_FILTER_SHIFT) + bldcInputPwm;
		bldcFilteredPwm = PWMFilterReg >> PWM_FILTER_SHIFT;

		// Update PWM channels based on position y(ellow), b(lue), g(reen)
		blockPWM(bldcFilteredPwm, pos, &y, &b, &g);
	}
		
	// Set PWM output (pwm_res/2 is the mean value, setvalue has to be between 10 and pwm_res-10)
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, CLAMP(g + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, CLAMP(b + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, CLAMP(y + pwm_res / 2, 10, pwm_res-10));
	
	// Safe last position
	lastPos = pos;
}


// PIDF coefficients are all scaled up by 15 bits (32768)
const int32_t KF      = (int32_t)(32768.0 *   0.16) ;
const int32_t KFO     = (int32_t)(32768.0 *  28.0) ;
const int32_t KP      = (int32_t)(32768.0 *   0.2) ;
const int32_t KI      = (int32_t)(32768.0 *   0.00005) ;
const int32_t ILIMIT  = (int32_t)(32768.0 * 150) ;


int16_t	runPID() {
	speedError = (speedSetpoint - realSpeedmmPS) ;
	
	// Determine Feed Forward and Proportional terms
	outF = ((speedSetpoint * KF) + (speedDir * KFO)) >> 15 ;
	outP = (speedError * KP) >> 15;

	// reset integral term if speed request near zero, or has changed signs.
	// otherwise intergrate error
	if ((abs32(speedSetpoint) < MIN_SPEED) ||
      ((speedSetpoint * lastSpeedSetpoint) < 0))	{
		errorIntegral = 0;
	} else {
		errorIntegral += (speedError * KI) ;
		errorIntegral = CLAMP(errorIntegral, (-ILIMIT), ILIMIT);  // do not let integral wind up too much
	}
	outI = errorIntegral >> 15;
	
	lastSpeedSetpoint = speedSetpoint;
	
	// Combine terms to form output
	return (outF + outP + outI) ;
}

uint8_t	hallToPos() {
	
  // Read hall sensors
	uint8_t hall_a = gpio_input_bit_get(HALL_A_PORT, HALL_A_PIN);
  uint8_t	hall_b = gpio_input_bit_get(HALL_B_PORT, HALL_B_PIN);
	uint8_t	hall_c = gpio_input_bit_get(HALL_C_PORT, HALL_C_PIN);
  
	// Determine current position based on hall sensors
  uint8_t hall = (hall_a * 1) + (hall_b * 2) + (hall_c * 4);
	
  return hall_to_pos[hall];
}

int16_t	getTransitionAngle() {
	int16_t Yangle ; 
	
	if (speedDir > 0)
    Yangle = (pos * 60) + TRANSITION_ANGLE;
	else	
    Yangle = (pos * 60) + 180 - TRANSITION_ANGLE;
	
	return (Yangle);
}


void	setPhaseAngle(int16_t Yangle) {

	step_y						= Yangle + PHASE_Y_OFFSET;
	step_b						= Yangle + PHASE_B_OFFSET;
	step_g						= Yangle + PHASE_G_OFFSET;
	
	// wrap into 0 359 range
	step_y = (step_y + FULL_PHASE) % FULL_PHASE;
	step_b = (step_b + FULL_PHASE) % FULL_PHASE;
	step_g = (step_g + FULL_PHASE) % FULL_PHASE;
}


int16_t	abs16 (int16_t value) {
	if (value < 0)
		value = -value;
	
	return value;
}

int32_t	abs32 (int32_t value) {
	if (value < 0)
		value = -value;
	
	return value;
}
