#### Updates:
````
- Firmware is ready.
- Sadly I can not support any issues. I'm not a student, less free time :)
````

### Hoverboard-Firmware-Hack-Gen2

Hoverboard Hack Firmware Generation 2 for the Hoverboard with the two Mainboards instead of the Sensorboards (See Pictures).

This repo contains open source firmware for generic Hoverboards with two mainboards. It allows you to control the hardware of the new version of hoverboards (like the Mainboard, Motors and Battery) with an arduino or some other steering device for projects like driving armchairs.

The structure of the firmware is based on the firmware hack of Niklas Fauth (https://github.com/NiklasFauth/hoverboard-firmware-hack/). Because of a different model of processor (GD32F130C8 instead of STM32F103) it was not possible to use the same firmware and it has to be written from scratch (Different hardware, different, pins, different registers :( )

- This project requires knowledge of the initial project linked above.
- At the current point I am not able to support any questions or issues - sorry!

---

#### Hardware
![otter](https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2/blob/master/Hardware_Overview_small.png)

The hardware has two main boards, which are different equipped. They are connected via USART. Additionally there are some LED PCB connected at X1 and X2 which signalize the battery state and the error state. There is an programming connector for ST-Link/V2 and they break out GND, USART/I2C, 5V on a second pinhead.

The reverse-engineered schematics of the mainboards can be found here:
https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2/blob/master/Schematics/HoverBoard_CoolAndFun.pdf


---

#### Flashing
The firmware is built with Keil (free up to 32KByte). To build the firmware, open the Keil project file which is includes in repository. Right to the STM32, there is a debugging header with GND, 3V3, SWDIO and SWCLK. Connect GND, SWDIO and SWCLK to your SWD programmer, like the ST-Link found on many STM devboards.

- If you never flashed your mainboard before, the controller is locked. To unlock the flash, use STM32 ST-LINK Utility or openOCD.
- To flash the STM32, use the STM32 ST-LINK Utility as well, ST-Flash utility or Keil by itself.
- Hold the powerbutton while flashing the firmware, as the controller releases the power latch and switches itself off during flashing
