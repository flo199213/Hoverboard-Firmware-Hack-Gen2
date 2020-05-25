
### Hoverboard Utility Gateway System (HUGS).

Based on: Hoverboard-Firmware-Hack-Gen2

### Purpose:
The HUGS project goal is to enable Hoverboards, or Hoverboard drive components to be re-purposed to provide low-cost mobility to other systems, such as assistive devices for the disabled, general purpose robots or other labor saving devices.
To implement this goal, new code will be developed and installed in existing hoverboards, to provide a generic purpose control protocol.  This protocol will be used by external devices to run one or more “hoverboard” type wheels.  

### Latest release:
A major modification to the base control strategy was adding several different motor control methods.

The basic method is a simple PWM power control where the the user requests a PWM level from 0 to 1000 and the comutation happens as required by the current sensed position.  The resulting speed is an open loop response.

The next method was a closed loop response using the resultant phase transition speed as feedback. A Feed forward control loop with proportional and integral gain was created to regulate the resulting speed.  Control input is an actual linear speed in the +/- 5MPS range, expressed in mmPS.

The third method is a low speed closed loop mode that uses a sinusoidal stepper motor sequence to drive the commutation at a fixed rate, depending on the requested speed.  The method optains higgly accurate speeds at rates less than about 200 mmPS.

The fourth and final mode is a hybrid of the last two which switches automatically between the low speed stepper mode and the PID control mode.

### Background:
The popular “Hoverboard” toy uses a somewhat unique drive system.  A powerful brushless DC motor has been integrated inside each of the small rubber wheels, and paired with a low cost motor controller.  This mechanical/electrical combination provides a compact drive system that produce precise motions, under considerable load, for relatively low cost.

Through reverse engineering the controller electronics and firmware, code is now available which can be modified to provide alternative operation of the drive system (no longer tied to a tilt-and-drive control system).

The goal of HUGS is to reprogram the motor controller to accept external control commands, and return wheel status information.  HUGS will permit individual wheels to be commanded to run at set speeds, and to move to specified positions.  Current speed and position information will be returned.
By providing an open control protocol, other groups can develop their own high level control logic to run the drive wheels for a myriad of applications.

This repo contains open source firmware that has been deveoped and tested on HOVER-1 Ultra Hoverboard

---

#### Hardware
My initial Hoverboard purchase was the Hover-1 Ultra.  I wanted to do the work on a board that could be purchased by name.
I chose this model because it looked like the company had several models and was likely to be a more reliable vendor than most "direct from China" vendors.

Ultimately my choice backfired on me because instead of the more common single board controller, this unit had two boards (one master and one slave).  So I had to work from a more limited development set of Gen2 hoverboard.

My most upsetting discovery was that when I was all done, I purchased a different model (Maverick vs Ultra) from the same supllier, and discovered that the Ultra code does not translate.   

Since I'm realy not able to reverse engineer this new hardware, this my be a throw-away hoverboard.

So "For the record" everything here has been developed on the Hover-1 Ultra unit, and pictures of the controller boards are in the Documentation folder of this repo.

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
