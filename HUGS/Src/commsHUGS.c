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

#define HUGS_MAX_DATA 3   // Max variable Data Length'
#define HUGS_EOM_OFFSET 7				// Location of EOM char based on variable data length
#define USART_HUGS_TX_BYTES (HUGS_MAX_DATA + 8)  // Max buffeer size including
#define USART_HUGS_RX_BYTES (HUGS_MAX_DATA + 8)  // start '/' and stop character '\n'

static bool 	 sHUGSRecord = FALSE;
extern uint8_t usartHUGS_rx_buf[USART_HUGS_RX_BUFFERSIZE];
static uint8_t sUSARTHUGSRecordBuffer[USART_HUGS_RX_BYTES];
static uint8_t sUSARTHUGSRecordBufferCounter = 0;

void CheckUSARTHUGSInput(uint8_t u8USARTBuffer[]);
void SendHUGSReply(void);
uint16_t CalcCRC(uint8_t *ptr, int count);

typedef enum {NOP = 0, RSP, ENA, DIS, SSP, SAP, SRP, DOG, XXX=0xFF} CMD_ID;
typedef enum {NOR = 0, SPE, APO, RPO, VOL, AMP, TMP} RSP_ID;

// Variables updated by HUGS Message
uint8_t	  HUGS_Destination = 0;
uint8_t	  HUGS_Sequence = 0;
CMD_ID    HUGS_CommandID = NOP;
RSP_ID		HUGS_ResponseID = NOR;
bool			HUGS_Enabled = FALSE;
bool			HUGS_ESTOP = FALSE;
uint32_t	HUGS_WatchDog = TIMEOUT_MS;


//----------------------------------------------------------------------------
// Update USART HUGS input
//----------------------------------------------------------------------------
void UpdateUSARTHUGSInput(void)
{
	uint8_t character = usartHUGS_rx_buf[0];
	uint8_t length;
	
	// Start character is captured, start record
	if (character == '/')
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
				// Completed message lemgth
				sUSARTHUGSRecordBufferCounter = 0;
				sHUGSRecord = FALSE;
			
				// Check input
				CheckUSARTHUGSInput (sUSARTHUGSRecordBuffer);
			}
		}
	}
}

//----------------------------------------------------------------------------
// Check USART HUGS input
//----------------------------------------------------------------------------
void CheckUSARTHUGSInput(uint8_t USARTBuffer[])
{
	// Auxiliary variables
	uint16_t crc;
	uint8_t	length = USARTBuffer[1];	


	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[length + HUGS_EOM_OFFSET ] != '\n')
	{
		return;
	}

	// Calculate CRC (first bytes up to, not including crc)
	crc = CalcCRC(USARTBuffer, length + 5 );
	crc = 0; //  remove this later
	
	// Check CRC
	/*
	if ( USARTBuffer[length + 5] != ((crc >> 8) & 0xFF) ||
		USARTBuffer[length + 6] != (crc & 0xFF))
	{
		return;
	}
	*/
	
	// command is valid.  Process it now
	HUGS_Destination  = USARTBuffer[2] & 0x0F;
	HUGS_Sequence     = (USARTBuffer[2] >> 4) & 0x0F;
	HUGS_CommandID		= (CMD_ID)USARTBuffer[3] ;
	HUGS_ResponseID		= (RSP_ID)USARTBuffer[4] ;
	
	switch(HUGS_CommandID) {
		case ENA:
			HUGS_Enabled = TRUE;
			HUGS_ESTOP = FALSE;
			SetEnable(SET);
			break;

		case DIS:
			HUGS_Enabled = FALSE;
			SetEnable(RESET);
		  break;
		
		case XXX:
			HUGS_ESTOP = TRUE;
			SetEnable(RESET);
		  break;

		case SSP:
			SetPWM((int8_t)USARTBuffer[5] * 10);
		  break;

		case DOG:
			SetPWM((int8_t)USARTBuffer[5] * 10);
		  break;

		default:
		  break;
	}

	// Send answer
	SendHUGSReply();
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
}
	
//----------------------------------------------------------------------------
// Send reply frame via USART
//----------------------------------------------------------------------------
void SendHUGSReply()
{
	uint8_t index = 0;
	uint16_t crc = 0;
	uint8_t buffer[USART_HUGS_TX_BYTES];
	
	
	// Calculate CRC
  crc = CalcCRC(buffer, index);
	buffer[0] = '/';
	buffer[1] = 0;
	buffer[2] = HUGS_Sequence << 4;
	buffer[3] = RSP;
	buffer[4] = HUGS_ResponseID;
  buffer[5] = (crc >> 8) & 0xFF;
  buffer[6] = crc & 0xFF;
	buffer[7] = '\n';

	SendBuffer(USART_HUGS, buffer, buffer[1] + 8);
}

