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

#include "SteeringSerial.h"

//----------------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------------
int32_t speedValue = 0;
int32_t steerValue = 0;
uint8_t upperLEDMaster = 0;
uint8_t lowerLEDMaster = 0;
uint8_t mosfetOutMaster = 0;
uint8_t upperLEDSlave = 0;
uint8_t lowerLEDSlave = 0;
uint8_t mosfetOutSlave = 0;
uint8_t beepsBackwards = 0;
uint8_t activateWeakening = 0;

void SendBuffer(uint8_t buffer[], uint8_t length);
uint16_t CalcCRC(uint8_t *ptr, int count);

//----------------------------------------------------------------------------
// Initializes the steering serial
//----------------------------------------------------------------------------
void InitSteeringSerial(void)
{
  // Set up serial communication
  Serial.begin(19200, SERIAL_8N1);
}

//----------------------------------------------------------------------------
// Sets the speed value
//----------------------------------------------------------------------------
void SetSpeed(uint16_t data, float factor)
{
  int16_t tempValue = ((float)data * 2 ) - 3000.0;  // Value -1000 to 1000
  tempValue *= factor;
  tempValue = CLAMP(tempValue, -1000, 1000);        // Avoid calculation failure
  speedValue = tempValue;
}

//----------------------------------------------------------------------------
// Sets the steering value
//----------------------------------------------------------------------------
void SetSteer(uint16_t data)
{
  int16_t tempValue = ((float)data * 2 ) - 3000.0;  // Value -1000 to 1000
  tempValue = CLAMP(tempValue, -1000, 1000);        // Avoid calculation failure
  if (speedValue < 0)
  {
    steerValue *= -1;
  }
  steerValue = tempValue;
}

//----------------------------------------------------------------------------
// Sends answer to master device
//----------------------------------------------------------------------------
void SendAnswer(void)
{
  int index = 0;
  uint8_t buffer[9];
  uint8_t byte1 = 0;
  uint8_t byte2 = 0;
  uint8_t byte3 = 0;
  uint8_t byte4 = 0;
  
  uint8_t sendByte = 0;
  sendByte |= (activateWeakening << 7);
  sendByte |= (beepsBackwards << 6);
  sendByte |= (mosfetOutSlave << 5);
  sendByte |= (lowerLEDSlave << 4);
  sendByte |= (upperLEDSlave << 3);
  sendByte |= (mosfetOutMaster << 2);
  sendByte |= (lowerLEDMaster << 1);
  sendByte |= (upperLEDMaster << 0);
  
  uint16_t speedValue_Format = (uint16_t)(speedValue);
  byte1 |= (speedValue_Format >> 8) & 0xFF;
  byte2 |= speedValue_Format & 0xFF;

  uint16_t steerValue_Format = (uint16_t)(steerValue);
  byte3 |= (steerValue_Format >> 8) & 0xFF;
  byte4 |= steerValue_Format & 0xFF;
  
  // Send answer
  buffer[index++] = '/';
  buffer[index++] = byte1;
  buffer[index++] = byte2;
  buffer[index++] = byte3;
  buffer[index++] = byte4;
  buffer[index++] = sendByte;

  // Calculate CRC
  uint16_t crc = CalcCRC(buffer, index);
  buffer[index++] = (crc >> 8) & 0xFF;
  buffer[index++] = crc & 0xFF;

  // Stop byte
  buffer[index++] = '\n';
  
  SendBuffer(buffer, index);
}

//----------------------------------------------------------------------------
// Calculates CRC value
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

//----------------------------------------------------------------------------
// Sends buffer
//----------------------------------------------------------------------------
void SendBuffer(uint8_t buffer[], uint8_t length)
{
  uint8_t index = 0;
  
  for(; index < length; index++)
  {
    Serial.write(buffer[index]);
  }
}

//----------------------------------------------------------------------------
// Sends debug infos
//----------------------------------------------------------------------------
void SendDebug()
{
  Serial.print(speedValue);
  Serial.print(",");
  Serial.println(steerValue);
}
