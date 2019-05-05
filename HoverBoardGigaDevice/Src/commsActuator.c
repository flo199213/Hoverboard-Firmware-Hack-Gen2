
#include "gd32f1x0.h"
#include "../Inc/it.h"
#include "../Inc/comms.h"
#include "../Inc/commsActuator.h"
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

// Only master control ACTUATOR (PWM output)
#ifdef MASTER

extern int32_t actuatorSpeed; // global variable for actuator speed.    -1000 to 1000

uint16_t counterPWMFrame=0;
bool signalIsHigh=FALSE;
//----------------------------------------------------------------------------
// Update Actuator Output each 31,25us
//----------------------------------------------------------------------------
void UpdateActuatorOutput(void)
{
	//maintain signal high for a time proportional to actuatorSpeed variable
	//1ms=0%, 2ms=100%
	//pwm frequency=50hz => time=20msec => counter shall reach 640
	int32_t signalHighTime= 1000+actuatorSpeed ; //time in usec
	signalHighTime=signalHighTime/31.25; //number of samples
	
	if(counterPWMFrame==640){
		
		gpio_bit_write(MOSFET_OUT_PORT, MOSFET_OUT_PIN, SET); //start frame
		signalIsHigh=TRUE;
		counterPWMFrame=0;
	}
	
	
	if((counterPWMFrame>=signalHighTime) && signalIsHigh){
		gpio_bit_write(MOSFET_OUT_PORT, MOSFET_OUT_PIN, RESET);  
		signalIsHigh=FALSE;
	}
	
	counterPWMFrame++;
	
}


#endif
