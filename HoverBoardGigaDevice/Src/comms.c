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
* could find ab better way through the code.
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
#include "../Inc/setup.h"
#include "../Inc/config.h"
#include "../Inc/defines.h"
#include "../Inc/bldc.h"
#include "stdio.h"
#include "string.h"

#ifdef MASTER
#define USART_MASTERSLAVE_TX_BYTES 14  // Transmit byte count including start and stop character '/'
#define USART_MASTERSLAVE_RX_BYTES 8   // Receive byte count including start and stop character '/'

#define USART_STEER_TX_BYTES 10  // Transmit byte count including start and stop character '/'
#define USART_STEER_RX_BYTES 9   // Receive byte count including start and stop character '/'
#endif
#ifdef SLAVE
#define USART_MASTERSLAVE_TX_BYTES 8   // Transmit byte count including start and stop character '/'
#define USART_MASTERSLAVE_RX_BYTES 14  // Receive byte count including start and stop character '/'

extern float currentDC;
#endif

extern uint8_t usartMasterSlave_rx_buf[USART_MASTERSLAVE_RX_BUFFERSIZE];
static uint8_t sMasterSlaveRecord = 0;
static uint8_t sUSARTMasterSlaveRecordBuffer[USART_MASTERSLAVE_RX_BYTES];
static uint8_t sUSARTMasterSlaveRecordBufferCounter = 0;

#ifdef MASTER
extern uint8_t usartSteer_rx_buf[USART_STEER_RX_BUFFERSIZE];
static uint8_t sSteerRecord = 0;
static uint8_t sUSARTSteerRecordBuffer[USART_STEER_RX_BYTES];
static uint8_t sUSARTSteerRecordBufferCounter = 0;

void UpdateUSARTSteerInput(void);
void CheckUSARTSteerInput(uint8_t u8USARTBuffer[]);

extern int32_t steer;
extern int32_t speed;
extern FlagStatus activateWeakening;
extern FlagStatus beepsBackwards;
extern float currentDCSlave;
extern uint8_t slaveError;
extern FlagStatus upperLEDSlave;
extern FlagStatus lowerLEDSlave;
extern FlagStatus mosfetOutSlave;
#endif

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
	// Auxiliary variables
	float digit1 = 0;
	float digit2 = 0;
	float decimalPlace1 = 0;
	float decimalPlace2 = 0;
	float decimalPlace3 = 0;
	
	// Calculate result current of slave (in ampere)
	digit1 = (USARTBuffer[1] - '0') * 10;
	digit2 = (USARTBuffer[2] - '0');
	decimalPlace1 = (USARTBuffer[3] - '0') * 0.1;
	decimalPlace2 = (USARTBuffer[4] - '0') * 0.01;
	decimalPlace3 = (USARTBuffer[5] - '0') * 0.001;
	currentDCSlave = digit1 + digit2 + decimalPlace1 + decimalPlace2 + decimalPlace3;
	
	// Calculate error number of slave (number 0-9)
	slaveError = USARTBuffer[6] - '0';
#endif
#ifdef SLAVE
	// Result variables
	int16_t pwmSlave = 0;
	int16_t weakSlave = 0;
	FlagStatus greenLED = RESET;
	FlagStatus orangeLED = RESET;
	FlagStatus redLED = RESET;
	FlagStatus upperLED = RESET;
	FlagStatus lowerLED = RESET;
	FlagStatus mosfetOut = RESET;
	FlagStatus enable = RESET;
	FlagStatus shutoff = RESET;
	
	// Auxiliary variables
	int16_t sign = 0;
	int16_t digit1 = 0;
	int16_t digit2 = 0;
	int16_t digit3 = 0;
	int16_t digit4 = 0;
	uint8_t byteTemp1;
	uint8_t byteTemp2;
	uint8_t byteTemp3;
	uint8_t byte;
	char charVal[6];
	uint8_t buffer[USART_MASTERSLAVE_TX_BYTES];
	
	// Calculate result pwm value for slave -1000 to 1000
	sign = USARTBuffer[1] == '-' ? -1 : 1;
	digit1 = (USARTBuffer[2] - '0') * 1000;
	digit2 = (USARTBuffer[3] - '0') * 100;
	digit3 = (USARTBuffer[4] - '0') * 10;
	digit4 = (USARTBuffer[5] - '0');
	pwmSlave = sign * (digit1 + digit2 + digit3 + digit4);
	
	// Calculate result weak value for slave 0 to 1000
	digit1 = (USARTBuffer[6] - '0') * 1000;
	digit2 = (USARTBuffer[7] - '0') * 100;
	digit3 = (USARTBuffer[8] - '0') * 10;
	digit4 = (USARTBuffer[9] - '0');
	weakSlave = digit1 + digit2 + digit3 + digit4;
	
	// Calculate setvalues for LED and enable
	byteTemp1 = (USARTBuffer[10] - '0') * 100;
	byteTemp2 = (USARTBuffer[11] - '0') * 10;
	byteTemp3 = (USARTBuffer[12] - '0');
	byte = byteTemp1 + byteTemp2 + byteTemp3;
	
	shutoff = (byte & BIT(7)) ? SET : RESET;
	mosfetOut = (byte & BIT(6)) ? SET : RESET;
	upperLED = (byte & BIT(5)) ? SET : RESET;
	lowerLED = (byte & BIT(4)) ? SET : RESET;
	greenLED = (byte & BIT(3)) ? SET : RESET;
	orangeLED = (byte & BIT(2)) ? SET : RESET;
	redLED = (byte & BIT(1)) ? SET : RESET;
	enable = (byte & BIT(0)) ? SET : RESET;
	
	if (shutoff == SET)
	{
		// Disable usart
		usart_deinit(USART_MASTERSLAVE);
		
		// Set pwm and enable to off
		SetEnable(RESET);
		SetPWM(0, 0);
		
		gpio_bit_write(SELF_HOLD_PORT, SELF_HOLD_PIN, RESET);
		while(1)
		{
			// Reload watchdog until device is off
			fwdgt_counter_reload();
		}
	}
	
	// Set functions according to the variables
	gpio_bit_write(MOSFET_OUT_PORT, MOSFET_OUT_PIN, mosfetOut);
	gpio_bit_write(UPPER_LED_PORT, UPPER_LED_PIN, upperLED);
	gpio_bit_write(LOWER_LED_PORT, LOWER_LED_PIN, lowerLED);
	gpio_bit_write(LED_GREEN_PORT, LED_GREEN, greenLED);
	gpio_bit_write(LED_ORANGE_PORT, LED_ORANGE, orangeLED);
	gpio_bit_write(LED_RED_PORT, LED_RED, redLED);
	SetEnable(enable);
	SetPWM(pwmSlave, weakSlave);
	
	// Send answer
  sprintf(charVal, "%06.3f", currentDC);
	buffer[0] = '/';
	buffer[1] = charVal[0]; // current DC tens
	buffer[2] = charVal[1]; // current DC ons
	buffer[3] = charVal[3]; // current DC first decimal place
	buffer[4] = charVal[4]; // current DC second decimal place
	buffer[5] = charVal[5]; // current DC third decimal place
	buffer[6] = '0';				// error number 0-9
	buffer[7] = '\n';				// end character
	
	SendBuffer(USART_MASTERSLAVE, buffer, USART_MASTERSLAVE_TX_BYTES);
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
#endif
}

#ifdef MASTER
//----------------------------------------------------------------------------
// Send slave frame via USART
//----------------------------------------------------------------------------
void SendPWMSlave(int16_t pwmSlave, int16_t weakSlave, FlagStatus enable, FlagStatus shutoff, FlagStatus greenLED, FlagStatus orangeLED, FlagStatus redLED, FlagStatus upperLED, FlagStatus lowerLED, FlagStatus mosfetOut)
{
	char charVal[4];
	uint8_t buffer[USART_MASTERSLAVE_TX_BYTES];
	
	int16_t sendPwm = CLAMP(pwmSlave, -1000, 1000);
	uint16_t sendPwmUint = ABS(pwmSlave);
	uint16_t sendweak = ABS(CLAMP(weakSlave, 0, 1000));
	
	uint8_t sendByte = 0;
	sendByte |= (shutoff << 7);
	sendByte |= (mosfetOut << 6);
	sendByte |= (upperLED << 5);
	sendByte |= (lowerLED << 4);
	sendByte |= (greenLED << 3);
	sendByte |= (orangeLED << 2);
	sendByte |= (redLED << 1);
	sendByte |= (enable << 0);
	
	// Send answer
  sprintf(charVal, "%04d", sendPwmUint);
	buffer[0] = '/';
	buffer[1] = sendPwm < 0 ? '-' : '+';
	buffer[2] = charVal[0];
	buffer[3] = charVal[1];
	buffer[4] = charVal[2];
	buffer[5] = charVal[3];
	sprintf(charVal, "%04d", sendweak);
	buffer[6] = charVal[0];
	buffer[7] = charVal[1];
	buffer[8] = charVal[2];
	buffer[9] = charVal[3];
	sprintf(charVal, "%03d", sendByte);
	buffer[10] = charVal[0];
	buffer[11] = charVal[1];
	buffer[12] = charVal[2];
	buffer[13] = '\n';
	
	SendBuffer(USART_MASTERSLAVE, buffer, USART_MASTERSLAVE_TX_BYTES);
}

//----------------------------------------------------------------------------
// Send frame to steer device
//----------------------------------------------------------------------------
void SendSteerDevice(steering_device_TX_buffer tx_buffer)
{
	int index = 0;
	uint8_t buffer[USART_STEER_TX_BYTES];
	uint16_t v_Batt_Format;
	uint16_t current_dc_Master_Format;
	uint16_t current_dc_Slave_Format;
	uint8_t byte1 = 0;
	uint8_t byte2 = 0;
	uint8_t byte3 = 0;
	uint8_t byte4 = 0;
	uint8_t byte5 = 0;
	uint8_t byte6 = 0;
	uint8_t byte7 = 0;
	
	uint8_t sendByte = 0;
	sendByte |= (tx_buffer.activateWeakening << 7);
	sendByte |= (tx_buffer.beepsBackwards << 6);
	sendByte |= (tx_buffer.mosfetOutSlave << 5);
	sendByte |= (tx_buffer.lowerLEDSlave << 4);
	sendByte |= (tx_buffer.upperLEDSlave << 3);
	sendByte |= (tx_buffer.mosfetOutMaster << 2);
	sendByte |= (tx_buffer.lowerLEDMaster << 1);
	sendByte |= (tx_buffer.upperLEDMaster << 0);
	
	// XX.XX * 100 = 0x0000XXXX ->(2^14)bit 0-16384
	v_Batt_Format = (uint16_t)(tx_buffer.v_batt * 100);
	byte1 |= v_Batt_Format >> 8 & 0xFF;
	byte2 |= v_Batt_Format & 0xFF;

	// XX.XX * 100 = 0x0000XXXX ->(2^14)bit 0-16384
	current_dc_Master_Format = (uint16_t)(tx_buffer.current_dc_Master * 100);
	byte3 |= current_dc_Master_Format >> 8 & 0xFF;
	byte4 |= current_dc_Master_Format & 0xFF;
	
	// XX.XX * 100 = 0x0000XXXX ->(2^14)bit 0-16384
	current_dc_Slave_Format = (uint16_t)(tx_buffer.current_dc_Slave * 100);
	byte5 |= current_dc_Slave_Format >> 8 & 0xFF;
	byte6 |= current_dc_Slave_Format & 0xFF;
	
	// XX 								->uint8_t (2^7) 0-128
	byte7 |= tx_buffer.realSpeed;
	
	// Daten: 57 bits = 8Bytes + S + S = 10Bytes
	buffer[index++] = '/';
	buffer[index++] = byte1;
	buffer[index++] = byte2;
	buffer[index++] = byte3;
	buffer[index++] = byte4;
	buffer[index++] = byte5;
	buffer[index++] = byte6;
	buffer[index++] = byte7;
	buffer[index++] = sendByte;
	buffer[index++] = '\n';
	
	SendBuffer(USART_STEER, buffer, index);
}

//----------------------------------------------------------------------------
// Update USART steer input
//----------------------------------------------------------------------------
void UpdateUSARTSteerInput(void)
{
	uint8_t character = usartSteer_rx_buf[0];
	
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
	// Result variables
	FlagStatus upperLEDMaster = RESET;
	FlagStatus lowerLEDMaster = RESET;
	FlagStatus mosfetOutMaster = RESET;
	
	// Auxiliary variables
	uint16_t crc;
	//uint16_t speedValue_Format;
	//uint16_t steerValue_Format;
	uint8_t byte;
	
	// Check start and stop character
	if ( USARTBuffer[0] != '/' ||
		USARTBuffer[USART_STEER_RX_BYTES - 1] != '\n')
	{
		return;
	}
	
	// Calculate CRC (first 6 bytes)
	crc = CalcCRC(USARTBuffer, 6);
	
	// Check CRC
	if ( USARTBuffer[6] != ((crc >> 8) & 0xFF) ||
		USARTBuffer[7] != (crc & 0xFF))
	{
		return;
	}
	
	// Calculate result speed value -1000 to 1000
	speed = (int16_t)((USARTBuffer[1] << 8) | USARTBuffer[2]);
	
	// Calculate result steering value -1000 to 1000
	steer = (int16_t)((USARTBuffer[3] << 8) | USARTBuffer[4]);
	
	// Calculate setvalues for LED and mosfets
	byte = USARTBuffer[5];
	
	activateWeakening = (byte & BIT(7)) ? SET : RESET;
	beepsBackwards = (byte & BIT(6)) ? SET : RESET;
	mosfetOutMaster = (byte & BIT(5)) ? SET : RESET;
	lowerLEDSlave = (byte & BIT(4)) ? SET : RESET;
	upperLEDSlave = (byte & BIT(3)) ? SET : RESET;
	mosfetOutMaster = (byte & BIT(2)) ? SET : RESET;
	lowerLEDMaster = (byte & BIT(1)) ? SET : RESET;
	upperLEDMaster = (byte & BIT(0)) ? SET : RESET;
	
	// Set functions according to the variables
	gpio_bit_write(MOSFET_OUT_PORT, MOSFET_OUT_PIN, mosfetOutMaster);
	gpio_bit_write(UPPER_LED_PORT, UPPER_LED_PIN, upperLEDMaster);
	gpio_bit_write(LOWER_LED_PORT, LOWER_LED_PIN, lowerLEDMaster);
	
	// Reset the pwm timout to avoid stopping motors
	ResetTimeout();
}
#endif

//----------------------------------------------------------------------------
// Send buffer via USART
//----------------------------------------------------------------------------
void SendBuffer(uint32_t usart_periph, uint8_t buffer[], uint8_t length)
{
	uint8_t index = 0;
	
	for(; index < length; index++)
	{
    usart_data_transmit(usart_periph, buffer[index]);
    while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET) {}
	}
}

//----------------------------------------------------------------------------
// Calculate CRC
//----------------------------------------------------------------------------
uint16_t CalcCRC(uint8_t *ptr, int count)
{
  uint16_t  crc;
  uint8_t i;
  crc = 0;
  while (--count >= 0)
  {
    crc = crc ^ (uint16_t) *ptr++ << 8;
    i = 8;
    do
    {
      if (crc & 0x8000)
      {
        crc = crc << 1 ^ 0x1021;
      }
      else
      {
        crc = crc << 1;
      }
    } while(--i);
  }
  return (crc);
}
