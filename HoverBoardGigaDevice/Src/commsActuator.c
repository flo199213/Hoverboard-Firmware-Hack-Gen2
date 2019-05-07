
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

uint32_t counterPWMFrame=0;
uint32_t counterTest=0;

bool signalIsHigh=FALSE;
float signalHighTime=0;
//----------------------------------------------------------------------------
// Update Actuator Output each 31,25us
//----------------------------------------------------------------------------
void UpdateActuatorOutput(void)
{

	//UNCOMMENT DURING DEBUG, JUST FOR TEST
	//if(counterTest>=64000){ //increment output each 2 second for test
	//	signalHighTime+=1;
	//	if(signalHighTime>310) signalHighTime=0;
	//	counterTest=0;
	//}
	//counterTest++;

	
	
	
	signalHighTime= 1000+actuatorSpeed ; //time in usec
	signalHighTime=signalHighTime/31.25; //number of samples
	
	//maintain signal high for a time proportional to actuatorSpeed variable
	//1ms=0%, 2ms=100%
	//pwm frequency=50hz => time=20msec => counter shall reach 640
	if(counterPWMFrame==640){
		//start frame
		gpio_bit_write(LED_GREEN_PORT, LED_GREEN, RESET); //(inverted logic since it uses npn transistor)
		signalIsHigh=TRUE;
		counterPWMFrame=0;
		
	}
	
	
	if((counterPWMFrame>=signalHighTime) && signalIsHigh){
		//end of the positive impulse
		gpio_bit_write(LED_GREEN_PORT, LED_GREEN, SET); //(inverted logic since it uses npn transistor)
		signalIsHigh=FALSE;
	}
	
	counterPWMFrame++;
	
}

	

#endif
