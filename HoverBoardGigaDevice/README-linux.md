# Building and flashing on Linux (GCC + OpenOCD)

This directory is the Keil uVision project of Hoverboard-Firmware-Hack-Gen2.
The files below add a plain GNU Make / arm-none-eabi-gcc build and an OpenOCD
flashing flow so the firmware can be built and flashed from Linux without Keil.

Target hardware: TT-SD2.2 "Gen2 split" boards (one MASTER, one SLAVE), each with
a **GD32F130C8T6** (Cortex-M3, 64 KiB flash, 8 KiB SRAM). The pin map in
`Inc/defines.h` is the authority for this board; in RoboDurden's Gen2.x
firmware the same board is layout **2.1.1** (formerly "2.0").

Files that make up the Linux port:

| file | purpose |
|------|---------|
| `Makefile` | build (`BOARD=master|slave`), `probe`, `unlock`, `verify-unlocked`, `flash`, `clean` |
| `GD32F130C8_FLASH.ld` | linker script: FLASH 64 KiB @ 0x08000000, RAM 8 KiB @ 0x20000000, `_estack = 0x20002000` |
| `startup_gd32f1x0.s` | GCC-syntax startup / vector table (the tracked `RTE/Device/GD32F130C8/startup_gd32f1x0.s` is Keil armasm syntax) |
| `RTE/Include/` | GD32F1x0 standard-peripheral headers + CMSIS core headers (Keil pulls these from its pack) |
| `Inc/gd32f1x0_libopt.h` | library option header required by `gd32f1x0.h` |
| `connect-under-reset.sh` | manual NRST-assisted attach helper (fallback; not needed once the hack firmware is on the board) |
| `tools/` | `build-all.sh`, `check-bins.sh`, `validate-checklist.md` |

Source patches needed for GCC (all tiny, see `git log`): `Inc/config.h` lets
`-DMASTER` / `-DSLAVE` from the command line override the hard-coded board
select; `Src/bldc.c` makes `blockPWM()` `static __INLINE` (GCC C99 `inline`
without `static` is not a definition); `Src/commsBluetooth.c` enlarges a
`char[5]` that `sprintf("%05d", int16)` overflows.

## 1. Toolchain (Ubuntu 24.04)

```sh
sudo apt install openocd stlink-tools gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```

Known-good versions: arm-none-eabi-gcc 13.2, OpenOCD 0.12, stlink-tools 1.7.
No Python is needed to build; the bench dashboard in `../../dashboard/` uses
the `../../.venv` (bleak + aiohttp).

## 2. Build

```sh
make BOARD=master        # -> build-master/hoverboard-master.{elf,hex,bin,map}
make BOARD=slave         # -> build-slave/hoverboard-slave.{elf,hex,bin,map}
make clean               # removes build-master/ and build-slave/
tools/build-all.sh       # both boards + sizes + sha256 + vector-table check
```

`BOARD` defaults to `master`. Flags: `-mcpu=cortex-m3 -mthumb -Os`,
`-DUSE_STDPERIPH_DRIVER -DGD32F130_150 -DMASTER|-DSLAVE`, newlib-nano, `--gc-sections`.
The build is deterministic: a clean rebuild reproduces byte-identical `.bin`
files (this is what `tools/build-all.sh` prints the sha256 for).

Reference sizes (upstream master `e138b22` + the patches above):

| board  | text  | data | bss  | .bin bytes |
|--------|-------|------|------|------------|
| master | 14408 | 20   | 1788 | 14428      |
| slave  | 17272 | 108  | 2136 | 17380      |

Run `tools/check-bins.sh` after any build. It verifies, without hardware, that
word 0 of each `.bin` is `0x20002000` (initial SP) and word 1 is an odd
(Thumb) reset vector inside `0x08000000..0x0800FFFF`, and cross-checks both
against the ELF.

## 3. Flashing — the procedure that worked on these boards

### 3.1 Wiring

ST-Link V2 on the **4-pin header next to the MCU**. Pin order on the header:

```
GND   D (SWDIO)   C (SWCLK)   3.3V
```

Only SWDIO/SWCLK/GND/3V3 are used; NRST is not on the header (see
`connect-under-reset.sh` if you ever need it).

OpenOCD is driven with `interface/stlink.cfg` + `target/stm32f1x.cfg` and
`reset_config none`. The GD32F130 has an STM32F1-compatible flash controller,
so the `stm32f1x` driver programs it fine. The chip identifies as
**DBGMCU_IDCODE (0xE0042000) = 0x13030410**, `flash size = 64 KiB`,
`Cortex-M3 r2p1`.

### 3.2 Virgin boards are read-protected (RDP)

Stock boards ship with RDP set. Symptoms: `mdw 0x4002201C` (FLASH_OBR) reads
`ffffff02` (bit1 = RDPRT), option bytes `mdw 0x1FFFF800 2` read `00ffa55a`,
and flash at `0x08000000` reads zeros/garbage. Clearing RDP **mass-erases the
chip** — the stock firmware is gone for good (it cannot be read out first).

```sh
make unlock              # openocd: init; halt; stm32f1x unlock 0   -> "stm32x unlocked."
# power-cycle the board  (option bytes only reload on power-up)
make verify-unlocked     # expect FLASH_OBR = ffffff00 and 0x08000000 = ffffffff ...
make flash BOARD=slave   # or BOARD=master; program + verify + reset run
```

After `unlock`, the option bytes read `00ff5aa5` immediately, but FLASH_OBR
stays `ffffff02` until the power cycle. Straight after the power cycle the
erased chip sits in HardFault at `pc 0xfffffffe` — normal for blank flash.
`make flash` prints `** Programming Finished ** / ** Verified OK **`; a
`Warn : Adding extra erase range ...` line is harmless (1 KiB page rounding).

### 3.3 Stock firmware blocks SWD on battery power — how attach was achieved

With the battery connected the stock firmware repurposes PA13/PA14, so
`openocd ... init` fails with `init mode failed (unable to connect to the
target)` thousands of times in a row, whatever you do with the power button.
What finally worked:

1. **Battery OUT.**
2. **Power the board from the ST-Link's 3V3 pin** (the 4th pin of the header).
   OpenOCD reports `Target voltage: ~3.26 V`.
3. Run an **attach-retry loop** on the host:

   ```sh
   while true; do
     openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "reset_config none" \
             -c "init; halt; mdw 0xE0042000; mdw 0x4002201C; mdw 0x1FFFF800 2; shutdown" \
       && break
   done
   ```

4. While the loop runs, **re-plug the ST-Link's USB**. Every re-plug
   power-cycles the board (it is fed from the ST-Link) at exactly the instant
   the freshly enumerated ST-Link starts probing, and the probe wins the race
   against the stock firmware's pin reconfiguration. It attached within a few
   re-plugs on both boards; the halted core showed `Handler HardFault, pc 0`.
5. With the session attached, do `make unlock`, power-cycle, `make verify-unlocked`,
   `make flash BOARD=...` as in 3.2 (a retry loop may be needed once more for
   the first attach after the power cycle).

After unlock and flashing **the hack firmware never touches PA13/PA14**, so
from then on SWD attaches normally (no loops, no re-plugging), including while
the firmware is running — this is what `../../dashboard/swd.py` relies on.

The Makefile comment about holding the power button applies to
battery-powered operation: `SELF_HOLD` (PB2) keeps the board on, and the
firmware's `ShutOff()` releases it. When the board is fed from the ST-Link 3V3
pin this does not matter.

## 4. Firmware behaviour worth knowing on the bench

### Battery thresholds and the 10S pack

`Inc/config.h` (MASTER only):

| define         | volts | effect |
|----------------|-------|--------|
| `BAT_LOW_LVL1` | 35.0  | green battery LED above this |
| `BAT_LOW_LVL2` | 33.0  | orange LED + gentle beeps below LVL1 |
| `BAT_LOW_DEAD` | 31.0  | red LED + louder beeps between DEAD and LVL2; **below 31.0 V the master calls `ShutOff()`** |

For a standard 10S Li-ion pack that is 3.1 V/cell (nominal 36 V, full 42 V).
`ShutOff()` beeps, sends the shut-off bit to the slave, deinitialises USART1,
disables the bridge, releases `SELF_HOLD` and then spins in
`while(1){ fwdgt_counter_reload(); }` — so the main loop (`steerCounter`)
freezes. Consequences:

- A bench supply below 31 V, or the board fed only from the ST-Link 3V3 pin
  (VBATT raw ~165 counts = ~4 V), makes the master shut itself off right after
  the power-button wait. That is expected, not a broken flash.
- `batteryVoltage` = raw ADC x `ADC_BATTERY_VOLT` (0.024169921875 V/count);
  `currentDC` uses `MOTOR_AMP_CONV_DC_AMP`.
- Also `ShutOff()` on: power button pressed again, or `INACTIVITY_TIMEOUT`
  (8 min) without |pwm| > 50 (unless the charger is detected on PF0).

### The three UARTs

| board  | USART | pins            | baud / format | protocol |
|--------|-------|-----------------|---------------|----------|
| master | USART0 (`USART_STEER_COM`) | PB6 TX / PB7 RX | 19200 8N1 | **steering**: master polls `"/\n"` (2 bytes) every 2nd main-loop pass; controller answers an 8-byte frame `'/' speed_hi speed_lo steer_hi steer_lo crc_hi crc_lo '\n'`, int16 big-endian -1000..1000, CRC-16/XMODEM (poly 0x1021, init 0) over the first 5 bytes (`Src/commsSteering.c`, `CalcCRC()` in `Src/comms.c`). A valid frame updates the globals `speed`/`steer` and resets the 2000 ms timeout; on timeout `TIMER13_IRQHandler` zeros both and sets `timedOut`. |
| slave  | USART0 (`USART_STEER_COM`) | PB6 TX / PB7 RX | 19200 8N1 | **11-byte ASCII telemetry / LED protocol** (`Src/commsBluetooth.c`): `'/' id id rw sign d d d d d '\n'`; `rw`='0' reads (ids 0-14: master battery x100, currents, real speeds, beep/LED state), `rw`='1' writes LED/beep settings. Reply has the same shape. |
| both   | USART1 (`USART_MASTERSLAVE`) | PA2 TX / PA3 RX | 115200 | **master<->slave link** (`Src/commsMasterSlave.c`): master -> slave 10 bytes (pwm int16, general-value id + int16, flag byte with enable/charge/shutoff bits, CRC), slave -> master 5 bytes (flag byte: upper/lower LED, mosfet out, beeps-backwards, CRC). Each valid frame resets the slave's 2000 ms timeout. |

Host-side references: `../../dashboard/protocol.py` (steering frame builder,
bit-exact CRC port) and `../../dashboard/swd.py` (reads `batteryVoltage`,
`currentDC`, `realSpeed`, `speed`, `steer`, `steerCounter`, `buzzerFreq` from
RAM via OpenOCD using addresses from the `.map`).

## 5. Bench validation

Follow `tools/validate-checklist.md` step by step (attach, RDP state, unlock,
flash + verify, heartbeat, battery ADC, steering frame accepted, timeout,
slave link). Append results to `../../flash.log`.

## 6. Git remotes used in this workspace

| remote | URL |
|--------|-----|
| `origin`     | https://github.com/ntindle/Hoverboard-Firmware-Hack-Gen2.git (fork) |
| `upstream`   | https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2.git |
| `gen2x`      | https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x.git |
| `gen2x-gd32` | https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x-GD32.git |
