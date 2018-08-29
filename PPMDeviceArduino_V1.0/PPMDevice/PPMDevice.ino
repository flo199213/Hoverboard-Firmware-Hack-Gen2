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

#include "RCReceiver.h"
#include "SteeringSerial.h"
#include "utils.h"

void setup()
{
  // Initialize steering serial
  InitSteeringSerial();

  // Enable Debug LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  // Initialize RC receiver
  InitRCReceiver();

  // Start RC receiver
  RCReceiverStart();
}

void loop()
{
  uint16_t channel1 = GetRCValue(0);
  uint16_t channel2 = GetRCValue(1);
  uint16_t channel3 = GetRCValue(2);
  
  // Activate/deactivate speed mode
  float factor = channel3 < 1500 ? 0.5 : 1;
  
  // Handle receiver values
  SetSpeed(channel1, factor);
  SetSteer(channel2);

/*  SendDebug(); */
  
  // Reply only when you receive data
  if (Serial.available() > 0)
  {
    char character = Serial.read();
    if (character == '\n')
    {
      SendAnswer();
    }
  }
}

