
#include "gd32f1x0.h"
#include "../Inc/it.h"
#include "../Inc/comms.h"
#include "../Inc/commsSteeringPWM.h"
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

// Only master receives PWM remote control INPUT
#ifdef MASTER

extern int32_t steer;
extern int32_t speed;
extern int32_t actuatorSpeed;


FlagStatus previousPWMSpeedSignalLevel=RESET; //false=low signal level, true=high signal level
FlagStatus previousPWMSteerSignalLevel=RESET; //false=low signal level, true=high signal level
float rc_speed_delay = 0;
float rc_steer_delay = 0;


uint8_t pwm_speed_count = 0;

#define IN_RANGE(x, low, up) (((x) >= (low)) && ((x) <= (up)))


//----------------------------------------------------------------------------
// read PPM input each 31,25us
//----------------------------------------------------------------------------
void CheckPWMRemoteControlInput(void)
{
	//stop counting SPEED on faling edge
	if (gpio_input_bit_get(SPEED_PWM_PORT, SPEED_PWM_PIN)==RESET && previousPWMSpeedSignalLevel==SET && rc_speed_delay<=2000){ //IF FALLING EDGE
		previousPWMSpeedSignalLevel=RESET;
		// Calculate result speed value -1000 to 1000
		// rc_speed_delay shall be between 1000 and 2000 microseconds.
		speed = (int16_t)CLAMP((rc_speed_delay-1500)*2.4, -1000, 1000);
		
		//if we are moving, controlled by RC, activate PWM output to control the blade of the lawnmower 
		if(speed>50 || speed<-50 ){
			actuatorSpeed=250;
		}else{
			actuatorSpeed=0;
		}
    // Reset the pwm timout to avoid stopping motors
		ResetTimeout();
		rc_speed_delay=0;
	}

		//stop counting STEER on faling edge
	if (gpio_input_bit_get(STEER_PWM_PORT, STEER_PWM_PIN)==RESET && previousPWMSteerSignalLevel==SET && rc_steer_delay<=2000){ //IF FALLING EDGE
		previousPWMSteerSignalLevel=RESET;
		// Calculate result speed value -1000 to 1000
		// rc_steer_delay shall be between 1000 and 2000 microseconds.
		steer = -(int16_t)CLAMP((rc_steer_delay-1500)*2.4, -1000, 1000);
    // Reset the pwm timout to avoid stopping motors ONLY ON SPEED. NOT NEEDED ON STEER
		//ResetTimeout();
		rc_steer_delay=0;
	}

	
	//start counting SPEED on rising edge
	if (gpio_input_bit_get(SPEED_PWM_PORT, SPEED_PWM_PIN)==SET && previousPWMSpeedSignalLevel==RESET){ //IF RISING EDGE
		previousPWMSpeedSignalLevel=SET;
		rc_speed_delay=0;
	}
	
	//start counting STEER on rising edge
	if (gpio_input_bit_get(STEER_PWM_PORT, STEER_PWM_PIN)==SET && previousPWMSteerSignalLevel==RESET){ //IF RISING EDGE
		previousPWMSteerSignalLevel=SET;
		rc_steer_delay=0;
	}
	
	
	rc_speed_delay=rc_speed_delay+31.25;
	rc_steer_delay=rc_steer_delay+31.25;

	//after 500ms declare the absence of pwm signal
	if(rc_speed_delay> 500000 || rc_steer_delay> 500000){
		#ifdef REMOTE_CONTROL_PWM
		speed = (int16_t)0;
		steer = (int16_t)0;
		actuatorSpeed=(int16_t)0;
		#endif
		rc_speed_delay=0;
		rc_steer_delay=0;
	}

}

	

#endif
