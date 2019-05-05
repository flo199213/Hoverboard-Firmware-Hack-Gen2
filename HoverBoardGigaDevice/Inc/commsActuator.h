

#ifndef COMMSACTUATOR_H
#define COMMSACTUATOR_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

// Only master controls ACTUATOR (PWM output)
#ifdef MASTER
void UpdateActuatorOutput(void);

#endif

#endif
