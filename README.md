
### Hoverboard Utility Gateway System (HUGS).

Based on: Hoverboard-Firmware-Hack-Gen2

### Purpose:
The HUGS project goal is to enable Hoverboards, or Hoverboard drive components to be re-purposed to provide low-cost mobility to other systems, such as assistive devices for the disabled, general purpose robots or other labor saving devices.
To implement this goal, new code will be developed and installed in existing hoverboards, to provide a generic purpose control protocol.  This protocol will be used by external devices to run one or more “hoverboard” type wheels.  

### Background:
The popular “Hoverboard” toy uses a somewhat unique drive system.  A powerful brushless DC motor has been integrated inside each of the small rubber wheels, and paired with a low cost motor controller.  This mechanical/electrical combination provides a compact drive system that produce precise motions, under considerable load, for relatively low cost.

Through reverse engineering the controller electronics and firmware, code is now available which can be modified to provide alternative operation of the drive system (no longer tied to a tilt-and-drive control system).

The goal of HUGS is to reprogram the motor controller to accept external control commands, and return wheel status information.  HUGS will permit individual wheels to be commanded to run at set speeds, and to move to specified positions.  Current speed and position information will be returned.
By providing an open control protocol, other groups can develop their own high level control logic to run the drive wheels for a myriad of applications.

This repo contains open source firmware that has been deveoped and tested on HOVER-1 Ultra Hoverboard

---

#### Hardware

The HOVER-1 Ultra has two sepparate motor controller boards, usually configured as a master and a slave with a USART Serial port passing commands and data back and forth.

There is a sets of pads that can be used to program the processor using a ST-Link/V2.

There is also a set of pads that can be used for a remote monitor /control port.

---

#### Flashing

The firmware is built with Keil (free up to 32KByte). 
To build the firmware, open the Keil project file (includes in repository).

- If the controller boards contain the original firmware, they will need to be unlocked. To do this, use the STM32 ST-LINK Utility.
- To flash the STM32, use the STM32 ST-LINK Utility, or use Keil and run the code in debug mode, which will download the code first.
- Hold the powerbutton while flashing the firmware, as the controller releases the power latch and switches itself off during flashing
