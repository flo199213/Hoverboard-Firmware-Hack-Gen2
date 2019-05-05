

#ifndef COMMSINTERLOCKS_H
#define COMMSINTERLOCKS_H

#include "gd32f1x0.h"
#include "../Inc/config.h"

// Only master HAS INTERLOCKS
#ifdef MASTER
//----------------------------------------------------------------------------
// Update INTERLOCK inputs
//----------------------------------------------------------------------------
void checkInterlockInputs(void);



#endif

#endif
