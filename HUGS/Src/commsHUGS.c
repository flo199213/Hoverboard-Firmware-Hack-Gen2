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
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

#define HUGS_MAX_DATA 9  			  // Max variable Data Length'
#define HUGS_EOM_OFFSET 7				// Location of EOM char based on variable data length
#define USART_HUGS_TX_BYTES (HUGS_MAX_DATA + 8)  // Max buffeer size including
#define USART_HUGS_RX_BYTES (HUGS_MAX_DATA + 8)  // start '/' and stop character '\n'

static bool 	  sHUGSRecord = FALSE;

extern uint8_t usartHUGS_rx_buf[USART_HUGS_RX_BUFFERSIZE];
static uint8_t sUSARTHUGSRecordBuffer[USART_HUGS_RX_BYTES];
static uint8_t sUSARTHUGSRecordBufferCounter = 0;

extern uint16_t batteryVoltagemV;
extern uint16_t currentDCmA     ;
extern int16_t  realSpeedmmPS   ;
extern int32_t  phasePeriod ;
extern int32_t  cycles      ;
extern int8_t   controlMode	;
extern uint8_t  speedMode ;
extern uint8_t  maxStepSpeed ;
extern int16_t  outF ;
extern int16_t  outP ;
extern int16_t  outI ;
extern int32_t  phasePeriod ;


bool CheckUSARTHUGSInput(uint8_t USARTBuffer[]);
void SendHUGSReply(void);
uint16_t CalcCRC(uint8_t *ptr, int count);
void ShutOff(void);

// Variables updated by HUGS Message
bool			HUGS_ESTOP = FALSE;
uint16_t	HUGS_WatchDog = 1000 ; // TIMEOUT_MS;
uint8_t	  HUGS_Destination = 0;
uint8_t	  HUGS_Sequence = 0;
CMD_ID    HUGS_CommandID = NOP;
RSP_ID		HUGS_ResponseID = NOR;


void SetESTOP(void) {
	HUGS_ESTOP = TRUE;
}

//----------------------------------------------------------------------------
// Update USART HUGS input
//----------------------------------------------------------------------------
void UpdateUSARTHUGSInput(void)
{
	uint8_t character = usartHUGS_rx_buf[0];
	uint8_t length;
	
	// Start character is captured, start record
	if (!sHUGSRecord && (character == '/'))
	{
		sUSARTHUGSRecordBufferCounter = 0;
		sHUGSRecord = TRUE;
	}

	// Process the new charcter
	if (sHUGSRecord)
	{
		sUSARTHUGSRecordBuffer[sUSARTHUGSRecordBufferCounter] = character;
		sUSARTHUGSRecordBufferCounter++;
		
	  // Check to see if we know the length yet
		if (sUSARTHUGSRecordBufferCounter > 1) {
			
			// Check for an invalid length, or a completed message
			if ((length = sUSARTHUGSRecordBuffer[1]) > HUGS_MAX_DATA){
				// Bad data length
				sUSARTHUGSRecordBufferCounter = 0;
				sHUGSRecord = FALSE;
			}
			else if (sUSARTHUGSRecordBufferCounter >  (length + HUGS_EOM_OFFSET))
			{
				// Check input
				if (CheckUSARTHUGSInput (sUSARTHUGSRecordBuffer)) {
					// A complete message was found.  Reset buffer and status
					sUSARTHUGSRecordBufferCounter = 0;
					sHUGSRecord = FALSE;
				} else {
					// Message was invalid.  it could have been a bad SOM
					// check to see if the buffer holds another SOM (/)
					int slider = 0;
					int ch;
					
					for (ch = 1; ch < sUSARTHUGSRecordBufferCounter; ch++) {
						if (sUSARTHUGSRecordBuffer[ch] == '/') {
							slider = ch;
							break;
						}
					}
					
					if (slider > 0) {
						// push the buffer back
						sUSARTHUGSRecordBufferCounter -= slider;
						memcpy(sUSARTHUGSRecordBuffer, sUSARTHUGSRecordBuffer + slider, sUSARTHUGSRecordBufferCounter);
					} else {
						sUSARTHUGSRecordBufferCounter = 0;
						sHUGSRecord = FALSE;
					}
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
// Check USART HUGS input
//----------------------------------------------------------------------------
bool CheckUSARTHUGSInput(uint8_t USARTBuffer[])
{
	// Auxiliary variables
	uint16_t crc;
	uint8_t	length = USARTBuffer[1];	

	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[length + HUGS_EOM_OFFSET ] != '\n')
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
	HUGS_Destination  = USARTBuffer[2] & 0x0F;
	HUGS_Sequence     = (USARTBuffer[2] >> 4) & 0x0F;
	HUGS_CommandID		= (CMD_ID)USARTBuffer[3] ;
	HUGS_ResponseID		= (RSP_ID)USARTBuffer[4] ;
	
	switch(HUGS_CommandID) {
		case ENA:
			SetEnable(SET);
			break;

		case DIS:
			SetEnable(RESET);
		  break;
		
		case POW:
			SetEnable(SET);  
			SetPower((int16_t)((uint16_t)USARTBuffer[6] << 8) +  (uint16_t)USARTBuffer[5]);
		  break;

		case DOG:
			// save the new watchdog count (in ms)
			HUGS_WatchDog = ((uint16_t)USARTBuffer[6] << 8) +  (uint16_t)USARTBuffer[5];
		  break;

		case MOD:
			// save the new speed Mode
			speedMode = USARTBuffer[5];
		  maxStepSpeed = USARTBuffer[6];
		  break;

		case RES:
			// reset the current wheel position
			cycles = 0;
		  break;

		case SPE:
			// Set the constant Speed (in mm/s)
			SetEnable(SET);  
			SetSpeed((int16_t)((uint16_t)USARTBuffer[6] << 8) +  (uint16_t)USARTBuffer[5]);
		  break;

		case XXX:
			// powerdown
			SetPower(0);
			SetESTOP();
			HUGS_ResponseID = STOP;
		  break;

		default:
		  break;
	}

	// Send answer
	if (HUGS_ResponseID != NOR) {
		SendHUGSReply();
	}
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
	
	return TRUE;
}


//----------------------------------------------------------------------------
// Send reply frame via USART
//----------------------------------------------------------------------------
void SendHUGSReply()
{
	uint8_t length = 1;
	uint16_t crc = 0;
	uint8_t buffer[USART_HUGS_TX_BYTES];
	int32_t positionMm;
	uint8_t bitStatus = HUGS_ESTOP ? 0x01 : 0x00;
	int16_t tempInt = 0;
	int32_t tempLong = 0;
	
	bitStatus |= (controlMode << 1);

	buffer[0] = '/';
	buffer[1] = 1;
	buffer[2] = HUGS_Sequence << 4;
	buffer[3] = RSP;
	buffer[4] = HUGS_ResponseID;
	buffer[5] = bitStatus;

	switch(HUGS_ResponseID) {
		case SSPE:
			  length = 3;
				buffer[6] = realSpeedmmPS & 0xFF ;
				buffer[7] = realSpeedmmPS >> 8;
			break;

		case SPOS:
			  length = 5;
				positionMm = GetPosition() ;
				buffer[6] = (positionMm) & 0xFF;
				buffer[7] = (positionMm >>  8) & 0xFF;
				buffer[8] = (positionMm >> 16) & 0xFF;
				buffer[9] = (positionMm >> 24) & 0xFF;
			break;

		case SVOL:
			  length = 3;
				buffer[6] = batteryVoltagemV & 0xFF ;
				buffer[7] = batteryVoltagemV >> 8;
			break;

		case SAMP:
			  length = 3;
				buffer[6] = currentDCmA & 0xFF ;
				buffer[7] = currentDCmA >> 8;
			break;

		case SPOW:
			  length = 3;
				tempInt = GetPWM();
				buffer[6] = tempInt & 0xFF ;
				buffer[7] = tempInt >> 8;
		
			break;
		
		case SDOG:
			  length = 3;
				buffer[6] = HUGS_WatchDog & 0xFF ;
				buffer[7] = HUGS_WatchDog >> 8;
			break;

		case SMOD:
			  length = 3;
				buffer[6] = speedMode ;
				buffer[7] = maxStepSpeed ;
		
			break;

		case SFPI:
			  length = 7;
				buffer[6]  = outF & 0xFF ;
				buffer[7]  = outF >> 8;
				buffer[8]  = outP & 0xFF ;
				buffer[9]  = outP >> 8;
				buffer[10] = outI & 0xFF ;
				buffer[11] = outI >> 8;
		
			break;

		case SMOT:
			  length = 9;
				tempInt = GetPWM();
				positionMm = GetPosition() ;

				buffer[6] = realSpeedmmPS & 0xFF ;
				buffer[7] = realSpeedmmPS >> 8;
		
				tempLong = positionMm;
				buffer[8]  = (tempLong) & 0xFF;
				buffer[9]  = (tempLong >>  8) & 0xFF;
				buffer[10] = (tempLong >> 16) & 0xFF;
				buffer[11] = (tempLong >> 24) & 0xFF;

				buffer[12] = tempInt & 0xFF ;
				buffer[13] = tempInt >> 8;
			break;

		case STOP:
			  length = 1;
			break;

		default:
			break;
	}

	buffer[1] = length;
	
	// Calculate CRC
  crc = CalcCRC(buffer, length + 5);
  buffer[length + 5] = crc & 0xFF;
  buffer[length + 6] = (crc >> 8) & 0xFF;
	buffer[length + 7] = '\n';
	
	SendBuffer(USART_HUGS, buffer, length + 8);
}

//----------------------------------------------------------------------------
// Send command via USART (Presumably to SLAVE)
//----------------------------------------------------------------------------
void SendHUGSCmd(CMD_ID SlaveCmd, int16_t value) 
{
	int8_t length = -1;
	uint16_t crc = 0;
	uint8_t buffer[USART_HUGS_TX_BYTES];
	
	switch(SlaveCmd) {
		case SPE:
			  length = 2;
				buffer[5] = value & 0xFF ;
				buffer[6] = value >> 8;
			break;

		case XXX:
			  length = 0;
			break;

		default:
			break;
	}

	// Only send a cmd if we have recognized the command and set a valid length
	if (length >= 0) {
		buffer[0] = '/';
		buffer[1] = length;
		buffer[2] = 0;
		buffer[3] = SlaveCmd;
		buffer[4] = NOR;
		
		// Calculate CRC
		crc = CalcCRC(buffer, length + 5);
		buffer[length + 5] = crc & 0xFF;
		buffer[length + 6] = (crc >> 8) & 0xFF;
		buffer[length + 7] = '\n';
		
		SendBuffer(USART_HUGS, buffer, length + 8);
	}
}

