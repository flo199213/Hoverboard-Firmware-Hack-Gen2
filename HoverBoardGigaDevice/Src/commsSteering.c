/*
* This file is part of the hoverboard-firmware-hack-V2 project. The 
* firmware is used to hack the generation 2 board of the hoverboard.
* These new hoverboards have no mainboard anymore. They consist of 
* two Sensorboards which have their own BLDC-Bridge per Motor and an
* ARM Cortex-M3 processor GD32F130C8.
*
* Copyright (C) 2018 Florian Staeblein
* Copyright (C) 2018 Jakob Broemauer
* Copyright (C) 2018 Kai Liebich
* Copyright (C) 2018 Christoph Lehnert
*
* The program is based on the hoverboard project by Niklas Fauth. The 
* structure was tried to be as similar as possible, so that everyone 
* could find a better way through the code.
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
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gd32f1x0.h"
#include "../Inc/it.h"
#include "../Inc/comms.h"
#include "../Inc/commsSteering.h"
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

// Only master communicates with steerin device
#ifdef MASTER
#define USART_STEER_TX_BYTES 2   // Transmit byte count including start '/' and stop character '\n'
#define USART_STEER_RX_BYTES 8   // Receive byte count including start '/' and stop character '\n'

extern uint8_t usartSteer_COM_rx_buf[USART_STEER_COM_RX_BUFFERSIZE];
static uint8_t sSteerRecord = 0;
static uint8_t sUSARTSteerRecordBuffer[USART_STEER_RX_BYTES];
static uint8_t sUSARTSteerRecordBufferCounter = 0;

void CheckUSARTSteerInput(uint8_t u8USARTBuffer[]);

extern int32_t steer;
extern int32_t speed;

//----------------------------------------------------------------------------
// Send frame to steer device
//----------------------------------------------------------------------------
void SendSteerDevice(void)
{
	int index = 0;
	uint8_t buffer[USART_STEER_TX_BYTES];
	
	// Ask for steer input
	buffer[index++] = '/';
	buffer[index++] = '\n';
	
	SendBuffer(USART_STEER_COM, buffer, index);
}

//----------------------------------------------------------------------------
// Update USART steer input
//----------------------------------------------------------------------------
void UpdateUSARTSteerInput(void)
{
	uint8_t character = usartSteer_COM_rx_buf[0];
	
	// Start character is captured, start record
	if (character == '/')
	{
		sUSARTSteerRecordBufferCounter = 0;
		sSteerRecord = 1;
	}

	if (sSteerRecord)
	{
		sUSARTSteerRecordBuffer[sUSARTSteerRecordBufferCounter] = character;
		sUSARTSteerRecordBufferCounter++;
		
		if (sUSARTSteerRecordBufferCounter >= USART_STEER_RX_BYTES)
		{
			sUSARTSteerRecordBufferCounter = 0;
			sSteerRecord = 0;
			
			// Check input
			CheckUSARTSteerInput (sUSARTSteerRecordBuffer);
		}
	}
}

//----------------------------------------------------------------------------
// Check USART steer input
//----------------------------------------------------------------------------
void CheckUSARTSteerInput(uint8_t USARTBuffer[])
{
	// Auxiliary variables
	uint16_t crc;
	
	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[USART_STEER_RX_BYTES - 1] != '\n')
	{
		return;
	}
	
	// Calculate CRC (first bytes except crc and stop byte)
	crc = CalcCRC(USARTBuffer, USART_STEER_RX_BYTES - 3);
	
	// Check CRC
	if ( USARTBuffer[USART_STEER_RX_BYTES - 3] != ((crc >> 8) & 0xFF) ||
		USARTBuffer[USART_STEER_RX_BYTES - 2] != (crc & 0xFF))
	{
		return;
	}
	
	// Calculate result speed value -1000 to 1000
	speed = (int16_t)((USARTBuffer[1] << 8) | USARTBuffer[2]);
	
	// Calculate result steering value -1000 to 1000
	steer = (int16_t)((USARTBuffer[3] << 8) | USARTBuffer[4]);
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
}
#endif
