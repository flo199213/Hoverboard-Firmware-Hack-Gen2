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
#include "../Inc/commsMasterSlave.h"
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

#ifdef MASTER
#define USART_MASTERSLAVE_TX_BYTES 10  // Transmit byte count including start '/' and stop character '\n'
#define USART_MASTERSLAVE_RX_BYTES 5   // Receive byte count including start '/' and stop character '\n'

// Variables which will be written by slave frame
extern FlagStatus beepsBackwards;
#endif
#ifdef SLAVE
#define USART_MASTERSLAVE_TX_BYTES 5   // Transmit byte count including start '/' and stop character '\n'
#define USART_MASTERSLAVE_RX_BYTES 10  // Receive byte count including start '/' and stop character '\n'

// Variables which will be send to master
FlagStatus upperLEDMaster = RESET;
FlagStatus lowerLEDMaster = RESET;
FlagStatus mosfetOutMaster = RESET;
FlagStatus beepsBackwardsMaster = RESET;

// Variables which will be written by master frame
int16_t currentDCMaster = 0;
int16_t batteryMaster = 0;
int16_t realSpeedMaster = 0;

void CheckGeneralValue(uint8_t identifier, int16_t value);
#endif

extern uint8_t usartMasterSlave_rx_buf[USART_MASTERSLAVE_RX_BUFFERSIZE];
static uint8_t sMasterSlaveRecord = 0;
static uint8_t sUSARTMasterSlaveRecordBuffer[USART_MASTERSLAVE_RX_BYTES];
static uint8_t sUSARTMasterSlaveRecordBufferCounter = 0;

void CheckUSARTMasterSlaveInput(uint8_t u8USARTBuffer[]);
void SendBuffer(uint32_t usart_periph, uint8_t buffer[], uint8_t length);
uint16_t CalcCRC(uint8_t *ptr, int count);

//----------------------------------------------------------------------------
// Update USART master slave input
//----------------------------------------------------------------------------
void UpdateUSARTMasterSlaveInput(void)
{
	uint8_t character = usartMasterSlave_rx_buf[0];
	
	// Start character is captured, start record
	if (character == '/')
	{
		sUSARTMasterSlaveRecordBufferCounter = 0;
		sMasterSlaveRecord = 1;
	}

	if (sMasterSlaveRecord)
	{
		sUSARTMasterSlaveRecordBuffer[sUSARTMasterSlaveRecordBufferCounter] = character;
		sUSARTMasterSlaveRecordBufferCounter++;
		
		if (sUSARTMasterSlaveRecordBufferCounter >= USART_MASTERSLAVE_RX_BYTES)
		{
			sUSARTMasterSlaveRecordBufferCounter = 0;
			sMasterSlaveRecord = 0;
			
			// Check input
			CheckUSARTMasterSlaveInput (sUSARTMasterSlaveRecordBuffer);
		}
	}
}

//----------------------------------------------------------------------------
// Check USART master slave input
//----------------------------------------------------------------------------
void CheckUSARTMasterSlaveInput(uint8_t USARTBuffer[])
{
#ifdef MASTER
	// Result variables
	FlagStatus upperLED = RESET;
	FlagStatus lowerLED = RESET;
	FlagStatus mosfetOut = RESET;
	
	// Auxiliary variables
	uint8_t byte;
#endif
#ifdef SLAVE
	// Result variables
	int16_t pwmSlave = 0;
	FlagStatus enable = RESET;
	FlagStatus shutoff = RESET;
	FlagStatus chargeStateLowActive = SET;
	
	// Auxiliary variables
	uint8_t identifier = 0;
	int16_t value = 0;
	uint8_t byte;
#endif
	// Auxiliary variables
	uint16_t crc;
	
	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[USART_MASTERSLAVE_RX_BYTES - 1] != '\n')
	{
		return;
	}
	
	// Calculate CRC (first bytes except crc and stop byte)
	crc = CalcCRC(USARTBuffer, USART_MASTERSLAVE_RX_BYTES - 3);
	
	// Check CRC
	if ( USARTBuffer[USART_MASTERSLAVE_RX_BYTES - 3] != ((crc >> 8) & 0xFF) ||
		USARTBuffer[USART_MASTERSLAVE_RX_BYTES - 2] != (crc & 0xFF))
	{
		return;
	}
	
#ifdef MASTER
	// Calculate setvalues for LED and mosfets
	byte = USARTBuffer[1];
	
	//none = (byte & BIT(7)) ? SET : RESET;
	//none = (byte & BIT(6)) ? SET : RESET;
	//none = (byte & BIT(5)) ? SET : RESET;
	//none = (byte & BIT(4)) ? SET : RESET;
	beepsBackwards = (byte & BIT(3)) ? SET : RESET;
	mosfetOut = (byte & BIT(2)) ? SET : RESET;
	lowerLED = (byte & BIT(1)) ? SET : RESET;
	upperLED = (byte & BIT(0)) ? SET : RESET;
	
	// Set functions according to the variables
	gpio_bit_write(MOSFET_OUT_PORT, MOSFET_OUT_PIN, mosfetOut);
	gpio_bit_write(UPPER_LED_PORT, UPPER_LED_PIN, upperLED);
	gpio_bit_write(LOWER_LED_PORT, LOWER_LED_PIN, lowerLED);
#endif
#ifdef SLAVE
	// Calculate result pwm value -1000 to 1000
	pwmSlave = (int16_t)((USARTBuffer[1] << 8) | USARTBuffer[2]);
	
	// Get identifier
	identifier = USARTBuffer[3];
	
	// Calculate result general value
	value = (int16_t)((USARTBuffer[4] << 8) | USARTBuffer[5]);
	
	// Calculate setvalues for enable and shutoff
	byte = USARTBuffer[6];
	
	shutoff = (byte & BIT(7)) ? SET : RESET;
	//none = (byte & BIT(6)) ? SET : RESET;
	//none = (byte & BIT(5)) ? SET : RESET;
	//none = (byte & BIT(4)) ? SET : RESET;
	//none = (byte & BIT(3)) ? SET : RESET;
	//none = (byte & BIT(2)) ? SET : RESET;
	chargeStateLowActive = (byte & BIT(1)) ? SET : RESET;
	enable = (byte & BIT(0)) ? SET : RESET;
	
	if (shutoff == SET)
	{
		// Disable usart
		usart_deinit(USART_MASTERSLAVE);
		
		// Set pwm and enable to off
		SetEnable(RESET);
		SetPWM(0);
		
		gpio_bit_write(SELF_HOLD_PORT, SELF_HOLD_PIN, RESET);
		while(1)
		{
			// Reload watchdog until device is off
			fwdgt_counter_reload();
		}
	}
	
	// Set functions according to the variables
	gpio_bit_write(LED_GREEN_PORT, LED_GREEN, chargeStateLowActive == SET ? SET : RESET);
	gpio_bit_write(LED_RED_PORT, LED_RED, chargeStateLowActive == RESET ? SET : RESET);
	SetEnable(enable);
	SetPWM(pwmSlave);
	CheckGeneralValue(identifier, value);
	
	// Send answer
	SendMaster(upperLEDMaster, lowerLEDMaster, mosfetOutMaster, beepsBackwardsMaster);
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
#endif
}

#ifdef MASTER
//----------------------------------------------------------------------------
// Send slave frame via USART
//----------------------------------------------------------------------------
void SendSlave(int16_t pwmSlave, FlagStatus enable, FlagStatus shutoff, FlagStatus chargeState, uint8_t identifier, int16_t value)
{
	uint8_t index = 0;
	uint16_t crc = 0;
	uint8_t buffer[USART_MASTERSLAVE_TX_BYTES];
	
	// Format pwmValue and general value
	int16_t sendPwm = CLAMP(pwmSlave, -1000, 1000);
	uint16_t sendPwm_Uint = (uint16_t)(sendPwm);
	uint16_t value_Uint = (uint16_t)(value);
	
	uint8_t sendByte = 0;
	sendByte |= (shutoff << 7);
	sendByte |= (0 << 6);
	sendByte |= (0 << 5);
	sendByte |= (0 << 4);
	sendByte |= (0 << 3);
	sendByte |= (0 << 2);
	sendByte |= (chargeState << 1);
	sendByte |= (enable << 0);
	
	// Send answer
	buffer[index++] = '/';
	buffer[index++] = (sendPwm_Uint >> 8) & 0xFF;
	buffer[index++] = sendPwm_Uint & 0xFF;
	buffer[index++] = identifier;
	buffer[index++] = (value_Uint >> 8) & 0xFF;
	buffer[index++] = value_Uint & 0xFF;	
	buffer[index++] = sendByte;
	
	// Calculate CRC
  crc = CalcCRC(buffer, index);
  buffer[index++] = (crc >> 8) & 0xFF;
  buffer[index++] = crc & 0xFF;

  // Stop byte
  buffer[index++] = '\n';
	
	SendBuffer(USART_MASTERSLAVE, buffer, index);
}
#endif
#ifdef SLAVE
//----------------------------------------------------------------------------
// Send master frame via USART
//----------------------------------------------------------------------------
void SendMaster(FlagStatus upperLEDMaster, FlagStatus lowerLEDMaster, FlagStatus mosfetOutMaster, FlagStatus beepsBackwards)
{
	uint8_t index = 0;
	uint16_t crc = 0;
	uint8_t buffer[USART_MASTERSLAVE_TX_BYTES];
	
	uint8_t sendByte = 0;
	sendByte |= (0 << 7);
	sendByte |= (0 << 6);
	sendByte |= (0 << 5);
	sendByte |= (0 << 4);
	sendByte |= (beepsBackwards << 3);
	sendByte |= (mosfetOutMaster << 2);
	sendByte |= (lowerLEDMaster << 1);
	sendByte |= (upperLEDMaster << 0);
	
	// Send answer
	buffer[index++] = '/';
	buffer[index++] = sendByte;
	
	// Calculate CRC
  crc = CalcCRC(buffer, index);
  buffer[index++] = (crc >> 8) & 0xFF;
  buffer[index++] = crc & 0xFF;

  // Stop byte
  buffer[index++] = '\n';
	
	SendBuffer(USART_MASTERSLAVE, buffer, index);
}

//----------------------------------------------------------------------------
// Checks input value from master to set value depending on identifier
//----------------------------------------------------------------------------
void CheckGeneralValue(uint8_t identifier, int16_t value)
{
	switch(identifier)
	{
		case 0:
			currentDCMaster = value;
			break;
		case 1:
			batteryMaster = value;
			break;
		case 2:
			realSpeedMaster = value;
			break;
		case 3:
			break;
		default:
			break;
	}
}

//----------------------------------------------------------------------------
// Returns current value sent by master
//----------------------------------------------------------------------------
int16_t GetCurrentDCMaster(void)
{
	return currentDCMaster;
}

//----------------------------------------------------------------------------
// Returns battery value sent by master
//----------------------------------------------------------------------------
int16_t GetBatteryMaster(void)
{
	return batteryMaster;
}

//----------------------------------------------------------------------------
// Returns realspeed value sent by master
//----------------------------------------------------------------------------
int16_t GetRealSpeedMaster(void)
{
	return realSpeedMaster;
}

//----------------------------------------------------------------------------
// Sets upper LED value which will be send to master
//----------------------------------------------------------------------------
void SetUpperLEDMaster(FlagStatus value)
{
	upperLEDMaster = value;
}

//----------------------------------------------------------------------------
// Returns upper LED value sent by master
//----------------------------------------------------------------------------
FlagStatus GetUpperLEDMaster(void)
{
	return upperLEDMaster;
}

//----------------------------------------------------------------------------
// Sets lower LED value which will be send to master
//----------------------------------------------------------------------------
void SetLowerLEDMaster(FlagStatus value)
{
	lowerLEDMaster = value;
}

//----------------------------------------------------------------------------
// Returns lower LED value sent by master
//----------------------------------------------------------------------------
FlagStatus GetLowerLEDMaster(void)
{
	return lowerLEDMaster;
}

//----------------------------------------------------------------------------
// Sets mosfetOut value which will be send to master
//----------------------------------------------------------------------------
void SetMosfetOutMaster(FlagStatus value)
{
	mosfetOutMaster = value;
}

//----------------------------------------------------------------------------
// Returns MosfetOut value sent by master
//----------------------------------------------------------------------------
FlagStatus GetMosfetOutMaster(void)
{
	return mosfetOutMaster;
}

//----------------------------------------------------------------------------
// Sets beepsBackwards value which will be send to master
//----------------------------------------------------------------------------
void SetBeepsBackwardsMaster(FlagStatus value)
{
	beepsBackwardsMaster = value;
}

//----------------------------------------------------------------------------
// Returns beepsBackwardsMaster value sent by master
//----------------------------------------------------------------------------
FlagStatus GetBeepsBackwardsMaster(void)
{
	return beepsBackwardsMaster;
}
#endif
