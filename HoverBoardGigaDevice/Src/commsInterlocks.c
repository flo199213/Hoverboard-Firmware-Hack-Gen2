
#include "gd32f1x0.h"
#include "../Inc/it.h"
#include "../Inc/comms.h"
#include "../Inc/commsInterlocks.h"
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

// Only master has interlocks
#ifdef MASTER

extern FlagStatus panicButtonPressed;
extern FlagStatus it_is_Raining;

 
	void checkInterlockInputs(void)
{
	panicButtonPressed=gpio_input_bit_get(PANIC_BUTTON_PORT, PANIC_BUTTON_PIN);
	it_is_Raining=gpio_input_bit_get(RAIN_SENSOR_PORT, RAIN_SENSOR_PIN);
}

#endif
