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
#include "../Inc/defines.h"
#include "../Inc/config.h"
#include "../Inc/bldc.h"
#include "../Inc/led.h"
#include "../Inc/commsMasterSlave.h"
#include "../Inc/commsSteering.h"
#include "../Inc/commsBluetooth.h"

uint32_t msTicks;
uint32_t timeoutCounter_ms = 0;
FlagStatus timedOut = SET;

#ifdef SLAVE
uint32_t hornCounter_ms = 0;
#endif

extern int32_t steer;
extern int32_t speed;
extern FlagStatus activateWeakening;
extern FlagStatus beepsBackwards;

//----------------------------------------------------------------------------
// SysTick_Handler
//----------------------------------------------------------------------------
void SysTick_Handler(void)
{
  msTicks++;
}

//----------------------------------------------------------------------------
// Resets the timeout to zero
//----------------------------------------------------------------------------
void ResetTimeout(void)
{
  timeoutCounter_ms = 0;
}

//----------------------------------------------------------------------------
// Timer13_Update_Handler
// Is called when upcouting of timer13 is finished and the UPDATE-flag is set
// -> period of timer13 running with 1kHz -> interrupt every 1ms
//----------------------------------------------------------------------------
void TIMER13_IRQHandler(void)
{	
	if (timeoutCounter_ms > TIMEOUT_MS)
	{
		// First timeout reset all process values
		if (timedOut == RESET)
		{
#ifdef MASTER
			steer = 0;
			speed = 0;
			beepsBackwards = RESET;
#endif
#ifdef SLAVE
			SetPWM(0);
#endif
		}
		
		timedOut = SET;
	}
	else
	{
		timedOut = RESET;
		timeoutCounter_ms++;
	}

#ifdef SLAVE
	if (hornCounter_ms >= 2000)
	{
		// Avoid horn to be activated longer than 2 seconds
		SetUpperLEDMaster(RESET);
	}
	else if (hornCounter_ms < 2000)
	{
		hornCounter_ms++;
	}
	
	// Update LED program
	CalculateLEDProgram();
#endif
	
	// Clear timer update interrupt flag
	timer_interrupt_flag_clear(TIMER13, TIMER_INT_UP);
}

//----------------------------------------------------------------------------
// Timer0_Update_Handler
// Is called when upcouting of timer0 is finished and the UPDATE-flag is set
// AND when downcouting of timer0 is finished and the UPDATE-flag is set
// -> pwm of timer0 running with 16kHz -> interrupt every 31,25us
//----------------------------------------------------------------------------
void TIMER0_BRK_UP_TRG_COM_IRQHandler(void)
{
	// Start ADC conversion
	adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
	
	// Clear timer update interrupt flag
	timer_interrupt_flag_clear(TIMER_BLDC, TIMER_INT_UP);
}

//----------------------------------------------------------------------------
// This function handles DMA_Channel0_IRQHandler interrupt
// Is called, when the ADC scan sequence is finished
// -> ADC is triggered from timer0-update-interrupt -> every 31,25us
//----------------------------------------------------------------------------
void DMA_Channel0_IRQHandler(void)
{
	// Calculate motor PWMs
	CalculateBLDC();
	
	#ifdef SLAVE
	// Calculates RGB LED
	CalculateLEDPWM();
	#endif
	
	if (dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_FTF))
	{
		dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_FTF);        
	}
}


//----------------------------------------------------------------------------
// This function handles DMA_Channel1_2_IRQHandler interrupt
// Is asynchronously called when USART0 RX finished
//----------------------------------------------------------------------------
void DMA_Channel1_2_IRQHandler(void)
{
	// USART steer/bluetooth RX
	if (dma_interrupt_flag_get(DMA_CH2, DMA_INT_FLAG_FTF))
	{
#ifdef MASTER
		// Update USART steer input mechanism
		UpdateUSARTSteerInput();
#endif
#ifdef SLAVE
		// Update USART bluetooth input mechanism
		UpdateUSARTBluetoothInput();
#endif
		dma_interrupt_flag_clear(DMA_CH2, DMA_INT_FLAG_FTF);        
	}
}


//----------------------------------------------------------------------------
// This function handles DMA_Channel3_4_IRQHandler interrupt
// Is asynchronously called when USART_SLAVE RX finished
//----------------------------------------------------------------------------
void DMA_Channel3_4_IRQHandler(void)
{
	// USART master slave RX
	if (dma_interrupt_flag_get(DMA_CH4, DMA_INT_FLAG_FTF))
	{
		// Update USART master slave input mechanism
		UpdateUSARTMasterSlaveInput();
		
		dma_interrupt_flag_clear(DMA_CH4, DMA_INT_FLAG_FTF);        
	}
}

//----------------------------------------------------------------------------
// Returns number of milliseconds since system start
//----------------------------------------------------------------------------
uint32_t millis()
{
	return msTicks;
}

//----------------------------------------------------------------------------
// Delays number of tick Systicks (happens every 10 ms)
//----------------------------------------------------------------------------
void Delay (uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks)
	{
		__NOP();
	}
}

//----------------------------------------------------------------------------
// This function handles Non maskable interrupt.
//----------------------------------------------------------------------------
void NMI_Handler(void)
{
}

//----------------------------------------------------------------------------
// This function handles Hard fault interrupt.
//----------------------------------------------------------------------------
void HardFault_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles Memory management fault.
//----------------------------------------------------------------------------
void MemManage_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles Prefetch fault, memory access fault.
//----------------------------------------------------------------------------
void BusFault_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles Undefined instruction or illegal state.
//----------------------------------------------------------------------------
void UsageFault_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles System service call via SWI instruction.
//----------------------------------------------------------------------------
void SVC_Handler(void)
{
}

//----------------------------------------------------------------------------
// This function handles Debug monitor.
//----------------------------------------------------------------------------
void DebugMon_Handler(void)
{
}

//----------------------------------------------------------------------------
// This function handles Pendable request for system service.
//----------------------------------------------------------------------------
void PendSV_Handler(void)
{
}
