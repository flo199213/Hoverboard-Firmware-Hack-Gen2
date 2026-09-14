# Bench validation checklist (TT-SD2.2 / GD32F130C8, MASTER + SLAVE)

Use this after building with `make BOARD=master` / `make BOARD=slave` (or
`tools/build-all.sh`). Steps marked **[HW]** need the ST-Link and the boards;
everything else is host-only. Tick each box in order; a failure at any step
means "stop and understand" before continuing.

Reference tooling (not copied here, link only):

- `../../dashboard/swd.py` — passive SWD telemetry reader. Starts OpenOCD
  (`interface/stlink.cfg` + `target/stm32f1x.cfg`, `reset_config none`), talks to
  its telnet port and does `mdw`/`mdb` reads of the firmware globals
  `batteryVoltage`, `currentDC`, `realSpeed`, `speed`, `steer`, `steerCounter`,
  `buzzerFreq`. Addresses come from `build-master/hoverboard-master.map`, so a
  rebuild does not break it. No halt, no writes.
- `../../dashboard/protocol.py` — builds/parses the 8-byte steering frame
  (`'/' speed_hi speed_lo steer_hi steer_lo crc_hi crc_lo '\n'`, CRC-16/XMODEM
  over the first 5 bytes) and is a bit-exact port of `CalcCRC()` in `Src/comms.c`.
- `README-linux.md` (one directory up) — wiring, unlock and flash procedure.

Symbols you will read over SWD (grep them from the `.map`, e.g.
`grep -E ' (steerCounter|speed|steer|batteryVoltage)$' build-master/hoverboard-master.map`):

| symbol          | type     | meaning                                                     |
|-----------------|----------|-------------------------------------------------------------|
| `steerCounter`  | uint32   | incremented once per main-loop pass (`DELAY_IN_MAIN_LOOP`=5 ms) — heartbeat |
| `speed`,`steer` | int32    | last accepted steering-frame values, -1000..1000             |
| `batteryVoltage`| float    | filtered VBATT in volts (raw ADC x 0.024169921875)           |
| `currentDC`     | float    | filtered DC current in amps                                  |
| `timedOut`      | uint8    | SET (1) when no valid steering frame for `TIMEOUT_MS`=2000 ms |
| `buzzerFreq`    | uint8    | 0 = silent                                                   |

---

## 0. Host-side, no hardware

- [ ] `tools/build-all.sh` completes for both boards, prints sizes and sha256.
- [ ] `tools/check-bins.sh` reports `RESULT: all checks passed`
      (word0 = `0x20002000`, odd reset vector inside `0x08000000..0x0800FFFF`).
- [ ] `python3 ../../dashboard/protocol.py` prints `self-test OK`.
- [ ] Note the sha256 of the `.bin` you are about to flash (goes in the log).

## 1. Attach **[HW]**

Wiring: ST-Link V2 on the 4-pin header next to the MCU, order
**GND, D (SWDIO), C (SWCLK), 3.3V**. Battery OUT; board powered from the
ST-Link's 3V3 pin.

- [ ] `make probe` attaches. Expect
      `Cortex-M3 r2p1 processor detected`, `mdw 0xE0042000` -> `13030410`
      (DBGMCU_IDCODE), `flash size = 64 KiB`.
- [ ] If it does not attach and the board still runs *stock* firmware: run an
      attach-retry loop (see README-linux.md, "attach-retry loop") and re-plug
      the ST-Link USB while it runs. Each re-plug power-cycles the board at the
      instant OpenOCD probes.
- [ ] Record: target voltage reported by OpenOCD (~3.26 V when fed from ST-Link).

## 2. RDP (read-protection) state **[HW]**

- [ ] `mdw 0x4002201C` (FLASH_OBR). bit1 = RDPRT: `...ff02` = protected,
      `...ff00` = unprotected.
- [ ] `mdw 0x1FFFF800 2` (option bytes). `00ffa55a` = RDP on (virgin board),
      `00ff5aa5` = RDP off.
- [ ] If protected: `mdw 0x08000000 4` reads zeros/garbage -> expected.
- [ ] Decide: protected board must be unlocked (step 3); this **mass-erases** it.

## 3. Unlock **[HW]** (only for a protected board)

- [ ] `make unlock` -> `stm32x unlocked.` and option bytes now `00ff5aa5`.
- [ ] Power-cycle the board (lift the 3V3 wire or re-plug the ST-Link USB).
      FLASH_OBR only reloads on power-up.
- [ ] `make verify-unlocked` -> FLASH_OBR `ffffff00`, flash reads `ffffffff ...`.
      (It may need the retry loop once; the erased chip sits in HardFault at
      pc `0xfffffffe`, which is fine.)

## 4. Flash + verify **[HW]**

- [ ] `make flash BOARD=slave` (or `master`) on the matching board.
      Expect `** Programming Finished **`, `** Verify Started **`, `** Verified OK **`.
      A `Warn : Adding extra erase range` line is normal (1 KiB page rounding).
- [ ] After `reset run`, re-attach and `mdw 0x08000000 2` ->
      `20002000 <reset vector>`; the reset vector must equal the one
      `tools/check-bins.sh` printed for that `.bin`.
- [ ] Record board (master/slave), `.bin` sha256, date in the flash log.

## 5. Post-flash liveness **[HW]**

The new firmware never touches PA13/PA14, so SWD now attaches normally at any
time (no retry tricks).

- [ ] Attach passively (`swd.py`, or `openocd ... -c init` + telnet `mdw`).
- [ ] MASTER: after the power-button press that ends the startup wait,
      `steerCounter` increases between two reads ~1 s apart (loop delay is
      5 ms, so expect a rate in the low hundreds per second; any increase
      proves the main loop runs).
- [ ] `buzzerFreq` returns to 0 after the startup melody.
- [ ] `steerCounter` frozen == main loop stuck. Usual cause: `ShutOff()` was
      called (battery < `BAT_LOW_DEAD`, button pressed again, or inactivity),
      which ends in `while(1){ fwdgt_counter_reload(); }`.

## 6. Battery ADC sanity **[HW]**

- [ ] With the real 10S pack connected: `batteryVoltage` between ~31 V and 42 V,
      and stable (+/- 0.3 V) across 10 samples.
- [ ] Raw check: `adc_buffer.v_batt` (uint16, see `.map`) x 0.024169921875 ~= `batteryVoltage`.
- [ ] Without the pack (ST-Link 3V3 only): raw ~165 (~4 V). This is *below*
      `BAT_LOW_DEAD` (31 V) so the MASTER will `ShutOff()` right after the
      button wait — `steerCounter` stops. That is expected, not a fault.
- [ ] `currentDC` ~ 0 A with motors idle.

## 7. Steering frame accepted **[HW]**

MASTER USART0 (PB6 TX / PB7 RX), 19200 8N1. The master polls with `"/\n"`
every second loop pass; you answer with an 8-byte frame from `protocol.py`.

- [ ] Send `steer_frame(300, 0)` at >= 5 Hz. Over SWD `speed` becomes 300,
      `steer` 0, `timedOut` 0.
- [ ] Send `steer_frame(0, -200)`: `steer` becomes -200.
- [ ] Send a frame with a corrupted CRC byte: `speed`/`steer` do NOT change.
- [ ] |value| < 50 is treated as 0 by the mixer (dead band) — expected.
- [ ] Motors stay disconnected or wheels off the ground for this whole section.

## 8. Timeout -> zeros **[HW]**

- [ ] Stop sending frames. Within `TIMEOUT_MS` = 2000 ms `timedOut` becomes 1
      and `speed` and `steer` read 0 (`TIMER13_IRQHandler` in `Src/it.c`).
- [ ] Resume frames: `timedOut` back to 0 and values follow again.

## 9. Slave link up **[HW]**

MASTER <-> SLAVE on USART1 (PA2 TX / PA3 RX), 115200. Master sends 10-byte
frames (`SendSlave()`: pwm, enable/shutoff/charge flags, one "general value"
such as battery x100), slave answers 5-byte frames carrying only LED/beep flags
(`Src/commsMasterSlave.c`). The master keeps **no** link-status variable, so
judge the link from the SLAVE side using `build-slave/hoverboard-slave.map`.

- [ ] Both boards powered, 4-pin master/slave cable connected, master past its
      button wait.
- [ ] On the SLAVE: `batteryMaster` (int16, master battery voltage x100) becomes
      non-zero and tracks the master's `batteryVoltage`.
- [ ] On the SLAVE: `timedOut` reads 0 while the master is up (each valid
      master frame calls `ResetTimeout()`); unplug the cable -> 1 within 2 s and
      `pwmSlave` forced to 0.
- [ ] On the SLAVE: `pwmSlave` follows the master's mixed right-wheel command
      when a steering frame with steer != 0 is applied in step 7.
- [ ] Master `ShutOff()` (button) also switches the slave off (shutoff bit 7 in
      byte 6 of the master->slave frame).
- [ ] SLAVE USART0 (PB6/PB7), 11-byte ASCII "bluetooth" protocol
      (`Src/commsBluetooth.c`): frame = `'/' id id rw sign d d d d d '\n'`
      (id = two ASCII digits, rw = '0' read / '1' write, sign = '+'/'-', five
      ASCII digits). Send `/000+00000\n` (read id 0) and expect the reply
      `/000+ddddd\n` = master battery voltage x100. Ids 1-4: master/slave
      current and real speed; 5-14: beep/LED settings (writable with rw='1').

## 10. Log

- [ ] Append to `../../flash.log`: date, board, sha256, IDCODE, RDP state
      before/after, verify result, heartbeat rate, battery voltage, steering
      test result.
