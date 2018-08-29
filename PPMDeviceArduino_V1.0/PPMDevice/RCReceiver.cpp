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

uint16_t m_pulseStart[SERVOS];     // Last measured pulse start for each input.
uint16_t m_pulseLength_us[SERVOS]; // Last measured pulse length for each input.
static uint8_t lastB = 0;
volatile static bool enable = false;
volatile static bool pinChangeActive = false;

uint16_t RCReceiverGetStartCount();
uint16_t RCReceiverGetStopCount(uint8_t p_servo);
uint16_t RCReceiverGetCnt();

//----------------------------------------------------------------------------
// Initializes the steering serial
//----------------------------------------------------------------------------
void InitRCReceiver(void)
{
  // Deinitialize timer1
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TIMSK1 = 0;
  OCR1A = 0;
  OCR1B = 0;
  TCNT1 = 0;
  
  // We use pin 8-10  PB0, PB1, PB2 as Servo input pins
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  
  // We use pin change interrupts to detect changes in the signal 
  // Only allow pin change interrupts for PB0-3 (digital pins 8-10)
  PCMSK0 |= (1 << PCINT0) | (1 << PCINT1) | (1 << PCINT2);
  
  // Enable pin change interrupt 0
  PCICR |= (1 << PCIE0);
}

//----------------------------------------------------------------------------
// Starts the receiver
//----------------------------------------------------------------------------
void RCReceiverStart()
{
  // Clean buffers
  for (uint8_t i = 0; i < SERVOS; ++i)
  {
    m_pulseStart[i]  = 0;
    m_pulseLength_us[i] = ZERO_US;
  }
  
  // Start Timer 1
  // Timer 1 counts up with 1MHz, so that 1 count = 1us
  // Overflow interrupt after 1000000MHz / 65536 = 0,065536s -> every 65,5ms
  TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11) | _BV(CS10))) | _BV(CS11);
  TIMSK1 |= _BV(TOIE1);
}

//----------------------------------------------------------------------------
// Returns the current result value of specified pin
//----------------------------------------------------------------------------
uint16_t GetRCValue(uint8_t pin)
{
  if (pin < 0 || pin > SERVOS)
  {
    return ZERO_US;
  }
  return m_pulseLength_us[pin];
}

//----------------------------------------------------------------------------
// Will be called when a reciever pin changes
//----------------------------------------------------------------------------
void RCReceiverPinChanged(uint8_t p_servo, bool p_high)
{
  if (p_high)
  {
    // Start of pulse
    m_pulseStart[p_servo] = RCReceiverGetStartCount();
  }
  else
  {
    // End of pulse
    m_pulseLength_us[p_servo] = CLAMP(RCReceiverGetStopCount(p_servo), 1000, 2000);
  }
}

//----------------------------------------------------------------------------
// Returns the start count of the timer
//----------------------------------------------------------------------------
uint16_t RCReceiverGetStartCount()
{
  uint16_t cnt = RCReceiverGetCnt();
  
  // Start of pulse
  return cnt;
}

//----------------------------------------------------------------------------
// Returns the stop count of the specified input pin
//----------------------------------------------------------------------------
uint16_t RCReceiverGetStopCount(uint8_t p_servo)
{
  uint16_t cnt = RCReceiverGetCnt();
   
  // End of pulse
  return (cnt - m_pulseStart[p_servo]) >> 1;
}

//----------------------------------------------------------------------------
// Returns the count value of the timer
//----------------------------------------------------------------------------
uint16_t RCReceiverGetCnt()
{
  uint8_t oldSREG = SREG;
  cli();
  uint16_t cnt = TCNT1;
  SREG = oldSREG;

  return cnt;
}

//----------------------------------------------------------------------------
// Interrupt service routine for timer1 overflow
//----------------------------------------------------------------------------
ISR(TIMER1_OVF_vect)
{
  sei();
  
  // Between two overflow interrupts (every 65,5ms) should be 
  // at least one pin change interrupt (receiver pwm period is 6ms)
  if (pinChangeActive)
  {
    enable = true;
  }
  else
  {
    m_pulseLength_us[0] = ZERO_US;
    m_pulseLength_us[1] = ZERO_US;
    m_pulseLength_us[2] = ZERO_US;
    enable = false;
  }
  pinChangeActive = false;
}

// 
//----------------------------------------------------------------------------
// Interrupt service routine for pin change port 0 interrupt
//----------------------------------------------------------------------------
ISR(PCINT0_vect)
{
  if (enable == false)
  {
    pinChangeActive = true;
    return;
  }
  pinChangeActive = true;
  uint8_t newB = PINB;

  // Bitwise XOR will set all bits that have changed
  uint8_t chgB = newB ^ lastB;
  lastB = newB;
  
  // Has any of the pins changed?
  if (chgB)
  {
    // Find out which pin has changed
    if (chgB & _BV(0))
    {
      RCReceiverPinChanged(0, newB & _BV(0));
    }
    if (chgB & _BV(1))
    {
      RCReceiverPinChanged(1, newB & _BV(1));
    }
    if (chgB & _BV(2))
    {
      RCReceiverPinChanged(2, newB & _BV(2));
    }
  }
}
