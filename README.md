I started my code from Florian Repository ( https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2 )
Florian rewrote the firmware for hoverboards composed by 2 boards, starting from Niklas repository ( https://github.com/NiklasFauth/hoverboard-firmware-hack/ )
Niklas rewrote the firmware for hoverboards composed by a big board and sensor boards.

First thing that I've done was to add PWM input to control the robot with a RC receiver, in order to let me do all the functional checks, before to implement automatic navigation algorithm.
I used turnigy rc receiver directly connected to the board, on the unused gpio.

Second thing that i've done was to add PWM output in order to control a ESC connected to a motor. The motor is connected to a blade. 

Every time the robot moves, the blade starts to rotate at constant speed.

Third thing that I. ve done was to make the mechanical project (you find it in the folder LawnMowerMEchanicalProject).
These are some images (the base is aluminium and the cover in polycarbonate):
![otter](https://github.com/gaucho1978/LAWNMOWER-ROBOT-from-Hoverboard-/blob/master/LawnmowerMechanicalProject/3D%20PROJECT%20-%20INVENTOR%202014/pictures/bottom.png)
![otter](https://github.com/gaucho1978/LAWNMOWER-ROBOT-from-Hoverboard-/blob/master/LawnmowerMechanicalProject/3D%20PROJECT%20-%20INVENTOR%202014/pictures/side.png)
![otter](https://github.com/gaucho1978/LAWNMOWER-ROBOT-from-Hoverboard-/blob/master/LawnmowerMechanicalProject/3D%20PROJECT%20-%20INVENTOR%202014/pictures/top.png)
![otter](https://github.com/gaucho1978/LAWNMOWER-ROBOT-from-Hoverboard-/blob/master/LawnmowerMechanicalProject/3D%20PROJECT%20-%20INVENTOR%202014/pictures/front.png)


NEXT things that I will do: 
- add sensors and interlocks to algorithm,
- create a docking station for automatic recharge, 
- realize the mechanical structure, 
- inplement safety for automatic stopping blade 
- implement automatic navigation algorithm.

--------
The hoverboard with 2 boards uses processor GD32F130C8 (instead of STM32F103 used on Niklas hoverboard) 

#### Hardware
![otter](https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2/blob/master/Hardware_Overview_small.png)

The hoverboard hardware has two main boards, which are different equipped. They are connected via USART. Additionally there are some LED PCB connected at X1 and X2 which signalize the battery state and the error state. There is an programming connector for ST-Link/V2 and they break out GND, USART/I2C, 5V on a second pinhead.

The reverse-engineered schematics of the mainboards can be found in the folder HoverboardSchematics

#### Flashing
The firmware is built with Keil (free up to 32KByte). To build the firmware, open the Keil project file which is includes in repository. On the board, close to ARM processor, there is a debugging header with GND, 3V3, SWDIO and SWCLK. Connect GND, SWDIO and SWCLK to your SWD programmer, like the ST-Link V2.

- If you never flashed your mainboard before, the controller is locked. To unlock the flash, use STM32 ST-LINK Utility or openOCD.
- To flash the processor, use the STM32 ST-LINK Utility as well, ST-Flash utility or Keil by itself.
- Hold the powerbutton while flashing the firmware, as the controller releases the power latch and switches itself off during flashing
