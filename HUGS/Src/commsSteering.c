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
#include "../Inc/it.h"
#include "../Inc/comms.h"
#include "../Inc/commsHUGS.h"
#include "../Inc/commsSteering.h"
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

#define STEER_MAX_DATA 4  			  // Max variable Data Length'
#define STEER_EOM_OFFSET 7				// Location of EOM char based on variable data length
#define USART_STEER_RX_BYTES (STEER_MAX_DATA + 8)  // start '/' and stop character '\n'
#define USART_STEER_TX_BYTES 2    // Max buffeer size including

extern uint8_t usartSteer_COM_rx_buf[USART_STEER_COM_RX_BUFFERSIZE];

static bool 	 sSteerRecord = FALSE;
static uint8_t sUSARTSteerRecordBuffer[USART_STEER_RX_BYTES];
static uint8_t sUSARTSteerRecordBufferCounter = 0;
static uint8_t 	steerReply[USART_STEER_TX_BYTES] = {'/', '\n'};

CMD_ID    Steer_CommandID  = NOP;
RSP_ID		Steer_ResponseID = NOR;

bool CheckUSARTSteerInput(uint8_t u8USARTBuffer[]);


//----------------------------------------------------------------------------
// Update USART steer input
//----------------------------------------------------------------------------
void UpdateUSARTSteerInput(void)
{
	uint8_t character = usartSteer_COM_rx_buf[0];
	uint8_t length;
	
	
	// Start character is captured, start record
	if (!sSteerRecord && (character == '/'))
	{
		sUSARTSteerRecordBufferCounter = 0;
		sSteerRecord = TRUE;
	}

	// Process the new charcter
	if (sSteerRecord)
	{
		sUSARTSteerRecordBuffer[sUSARTSteerRecordBufferCounter] = character;
		sUSARTSteerRecordBufferCounter++;
		
	  // Check to see if we know the length yet
		if (sUSARTSteerRecordBufferCounter > 1) {
			
			// Check for an invalid length, or a completed message
			if ((length = sUSARTSteerRecordBuffer[1]) > STEER_MAX_DATA){
				// Bad data length
				sUSARTSteerRecordBufferCounter = 0;
				sSteerRecord = FALSE;
			}
			else if (sUSARTSteerRecordBufferCounter >  (length + STEER_EOM_OFFSET))
			{
				
				// Check input using HUGS message processing
				if (CheckUSARTSteerInput (sUSARTSteerRecordBuffer)) {

					// FOR TEST PURPOSES
					// SendBuffer(USART_STEER_COM, sUSARTSteerRecordBuffer, sUSARTSteerRecordBufferCounter);
					
					// A complete message was found.  Reset buffer and status
					sUSARTSteerRecordBufferCounter = 0;
					sSteerRecord = FALSE;
				} else {
					// Message was invalid.  it could have been a bad SOM
					// check to see if the buffer holds another SOM (/)
					int slider = 0;
					int ch;
					
					for (ch = 1; ch < sUSARTSteerRecordBufferCounter; ch++) {
						if (sUSARTSteerRecordBuffer[ch] == '/') {
							slider = ch;
							break;
						}
					}
					
					if (slider > 0) {
						// push the buffer back
						sUSARTSteerRecordBufferCounter -= slider;
						memcpy(sUSARTSteerRecordBuffer, sUSARTSteerRecordBuffer + slider, sUSARTSteerRecordBufferCounter);
					} else {
						sUSARTSteerRecordBufferCounter = 0;
						sSteerRecord = FALSE;
					}
				}
			}
		}
	}
	
}

//----------------------------------------------------------------------------
// Check USART steer input
//----------------------------------------------------------------------------
bool CheckUSARTSteerInput(uint8_t USARTBuffer[])
{
	// Auxiliary variables
	uint16_t  crc;
	uint8_t	  length = USARTBuffer[1];	
	int16_t		turn_mmPS;
	int16_t		left_mmPS;
	int16_t		right_mmPS;
	int16_t		smax;

	
	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[length + STEER_EOM_OFFSET ] != '\n')
	{
		return FALSE;
	}

	// Calculate CRC (first bytes up to, not including crc)
	crc = CalcCRC(USARTBuffer, length + 5 );
	
	// Check CRC
	if ( USARTBuffer[length + 5] != (crc & 0xFF) ||
		   USARTBuffer[length + 6] != ((crc >> 8) & 0xFF) )
	{
		return FALSE;
	}
	
	// command is valid.  Process it now
	Steer_CommandID		= (CMD_ID)USARTBuffer[3] ;
	Steer_ResponseID	= (RSP_ID)USARTBuffer[4] ;
	
	switch(Steer_CommandID) {
		case DSPE:
	
			// Set the constant Speed (in mm/s)
			SetEnable(SET); 
		
			left_mmPS  = (int16_t)((uint16_t)USARTBuffer[6] << 8) +  (uint16_t)USARTBuffer[5];
			right_mmPS = left_mmPS;

			turn_mmPS  = (int16_t)((float)((int16_t)((uint16_t)USARTBuffer[8] << 8) +  (uint16_t)USARTBuffer[7]) * MM_PER_DEGREE);

			left_mmPS  += turn_mmPS;
			right_mmPS -= turn_mmPS;

			// normalize so no speed is > +/-TOP_SPEED
			smax = max(abs16(left_mmPS), abs16(right_mmPS));

			if (smax > MAX_SPEED) {
				left_mmPS  = (left_mmPS  * MAX_SPEED) / smax;
				right_mmPS = (right_mmPS * MAX_SPEED) / smax;
			}

			SendHUGSCmd(SPE, -left_mmPS) ;
			SetSpeed(right_mmPS);
		  break;

		case XXX:
			// powerdown
		  SendHUGSCmd(SPE, 0) ;
		  SendHUGSCmd(XXX, 0) ;
			SetPower(0);
			SetESTOP();
		  break;

		default:
		  break;
	}

	// Send keep-alive reply
	SendBuffer(USART_STEER_COM, steerReply,  sizeof(steerReply));
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
	
	return TRUE;
}

int16_t		max(int16_t a, int16_t b){
	return (a >= b)? a : b;
}
