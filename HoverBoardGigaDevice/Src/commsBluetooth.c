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
#include "../Inc/defines.h"
#include "../Inc/setup.h"
#include "../Inc/comms.h"
#include "../Inc/commsMasterSlave.h"
#include "../Inc/commsBluetooth.h"
#include "../Inc/led.h"
#include "stdio.h"
#include "string.h"

// Only slave communicates over bluetooth
#ifdef SLAVE
// Variables which will be send over bluetooth
extern float currentDC;
extern float realSpeed;

extern uint32_t hornCounter_ms;

#define USART_BLUETOOTH_TX_BYTES 11   // Transmit byte count including start '/' and stop character '\n'
#define USART_BLUETOOTH_RX_BYTES 11   // Receive byte count including start '/' and stop character '\n'

extern uint8_t usartSteer_COM_rx_buf[USART_STEER_COM_RX_BUFFERSIZE];
static uint8_t sBluetoothRecord = 0;
static uint8_t sUSARTBluetoothRecordBuffer[USART_BLUETOOTH_RX_BYTES];
static uint8_t sUSARTBluetoothRecordBufferCounter = 0;

void CheckUSARTBluetoothInput(uint8_t USARTBuffer[]);
void SendBluetoothDevice(uint8_t identifier, int16_t value);

//----------------------------------------------------------------------------
// Update USART bluetooth input
//----------------------------------------------------------------------------
void UpdateUSARTBluetoothInput(void)
{
	uint8_t character = usartSteer_COM_rx_buf[0];
	
	// Start character is captured, start record
	if (character == '/')
	{
		sUSARTBluetoothRecordBufferCounter = 0;
		sBluetoothRecord = 1;
	}

	if (sBluetoothRecord)
	{
		sUSARTBluetoothRecordBuffer[sUSARTBluetoothRecordBufferCounter] = character;
		sUSARTBluetoothRecordBufferCounter++;
		
		if (sUSARTBluetoothRecordBufferCounter >= USART_BLUETOOTH_RX_BYTES)
		{
			sUSARTBluetoothRecordBufferCounter = 0;
			sBluetoothRecord = 0;
			
			// Check input
			CheckUSARTBluetoothInput (sUSARTBluetoothRecordBuffer);
		}
	}
}

//----------------------------------------------------------------------------
// Check USART bluetooth input
//----------------------------------------------------------------------------
void CheckUSARTBluetoothInput(uint8_t USARTBuffer[])
{
	// Auxiliary variables
	uint8_t identifier = 0;
	uint8_t readWrite = 0;
	int16_t sign = 0;
	int16_t digit1 = 0;
	int16_t digit2 = 0;
	int16_t digit3 = 0;
	int16_t digit4 = 0;
	int16_t digit5 = 0;
	int16_t value = 0;
	
	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[USART_BLUETOOTH_RX_BYTES - 1] != '\n')
	{
		return;
	}
	
	// Calculate identifier (number 0-99)
	digit1 = (USARTBuffer[1] - '0') * 10;
	digit2 = (USARTBuffer[2] - '0');
	identifier = digit1 + digit2;
	
	// Calculate read or write access (0 - read, 1 - write)
	readWrite = USARTBuffer[3] - '0';
	
	// If read mode, answer with correct value
	if (readWrite == 0)
	{
		switch(identifier)
		{
			case 0:
				// Answer with battery voltage from master
				value = GetBatteryMaster();
				break;
			case 1:
				// Answer with current from master
				value = GetCurrentDCMaster();
				break;
			case 2:
				// Answer with current from slave
				value = currentDC * 100;
				break;
			case 3:
				// Answer with real speed of master
				value = GetRealSpeedMaster();
				break;
			case 4:
				// Answer with real speed of slave
				value = realSpeed * 100;
				break;
			case 5:
				// Answer with beeps backwards from master
			  value = GetBeepsBackwardsMaster() == RESET ? 0 : 1;
				break;
			case 6:
				// Answer with lower LED state of master (music box)
				value = GetLowerLEDMaster() == RESET ? 0 : 1;
				break;
			case 7:
				// Answer with upper LED state of master (horn)
				value = GetUpperLEDMaster() == RESET ? 0 : 1;
				break;
			case 8:
				// Answer with hue value
				value = GetHSBHue();
				break;
			case 9:
				// Answer with saturation value
				value = GetHSBSaturation();
				break;
			case 10:
				// Answer with brightness value
				value = GetHSBBrightness();
				break;
			case 11:
				// Answer with LEDMode
				value = GetRGBProgram();
				break;
			case 12:
				// Answer with fading speed
				value = GetSpeedFading();
				break;
			case 13:
				// Answer with blink speed
				value = GetSpeedBlink();
				break;
			case 14:
				// Answer with strobe speed
				value = GetSpeedStrobe();
				break;
		}
		
		// Send Answer
		SendBluetoothDevice(identifier, value);
	}
	// If write mode, get result value
	else if ( readWrite == 1)
	{
		// Calculate result value (-10000 to 10000)
		sign = USARTBuffer[4] == '-' ? -1 : 1;
		digit1 = (USARTBuffer[5] - '0') * 10000;
		digit2 = (USARTBuffer[6] - '0') * 1000;
		digit3 = (USARTBuffer[7] - '0') * 100;
		digit4 = (USARTBuffer[8] - '0') * 10;
		digit5 = (USARTBuffer[9] - '0');
		value = sign * (digit1 + digit2 + digit3 + digit4 + digit5);
		
		switch(identifier)
		{
			case 5:
				SetBeepsBackwardsMaster(value == 0 ? RESET : SET);
				break;
			case 6:
				// Set lower LED of master (music box)
				SetLowerLEDMaster(value == 0 ? RESET : SET);
				break;
			case 7:
				// Set upper LED of master (horn)
				hornCounter_ms = 0;
				SetUpperLEDMaster(value == 0 ? RESET : SET);
				break;
			case 8:
				// Set LED hue
				SetHSBHue(value);
				break;
			case 9:
				// Set LED saturation
				SetHSBSaturation(value);
				break;
			case 10:
				// Set LED brightness
				SetHSBBrightness(value);
				break;
			case 11:
				// Set LED mode
				SetRGBProgram((LED_PROGRAM)value);
				break;
			case 12:
				// Set fading speed
				SetSpeedFading(value);
				break;
			case 13:
				// Set blink speed
				SetSpeedBlink(value);
				break;
			case 14:
				// Set strobe speed
				SetSpeedStrobe(value);
				break;
			default:
				// Do nothing for the rest of the identifiers
				break;
		}
	}
}

//----------------------------------------------------------------------------
// Send frame to bluetooth device
//----------------------------------------------------------------------------
void SendBluetoothDevice(uint8_t identifier, int16_t value)
{
	int index = 0;
	char charVal[5];
	uint8_t buffer[USART_BLUETOOTH_TX_BYTES];
	
	// Send bluetooth frame
	buffer[index++] = '/';
	sprintf(charVal, "%02d", identifier);
	buffer[index++] = charVal[0];
	buffer[index++] = charVal[1];
	buffer[index++] = '0';
	sprintf(charVal, "%05d", value);
	buffer[index++] = value < 0 ? '-' : '+';
	buffer[index++] = charVal[0];
	buffer[index++] = charVal[1];
	buffer[index++] = charVal[2];
	buffer[index++] = charVal[3];
	buffer[index++] = charVal[4];
	buffer[index++] = '\n';
	
	SendBuffer(USART_STEER_COM, buffer, index);
}

#endif
