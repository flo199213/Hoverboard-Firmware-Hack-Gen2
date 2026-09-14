#!/usr/bin/env bash
# Manual connect-under-reset for boards whose stock firmware disables SWD.
#
#   1. Wire the MCU's NRST pin to GND (hold it there).
#   2. Run this script. It attaches to the debug port while the core is held
#      in reset, then arms a "halt on reset vector" catch.
#   3. When it prints RELEASE NRST NOW, lift the NRST wire. The core boots,
#      halts on the very first instruction, and SWD stays alive.
#   4. Then run:  make unlock   /   make flash BOARD=...   (hold power button)
#
# Usage: ./connect-under-reset.sh [seconds-to-wait]   (default 20)
set -u
WAIT_MS=$(( ${1:-20} * 1000 ))
exec > >(tee -a /home/nick/code/hoverboard/flash.log) 2>&1
exec openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "adapter speed 300" \
  -c "reset_config none" \
  -c "init" \
  -c "echo {=== attached under reset; arming reset-vector catch ===}" \
  -c "mww 0xE000EDF0 0xA05F0001" \
  -c "mww 0xE000EDFC 0x01000001" \
  -c "echo {>>>>>>>>>> RELEASE NRST NOW (you have $(( WAIT_MS/1000 ))s) <<<<<<<<<<}" \
  -c "sleep $WAIT_MS" \
  -c "halt" \
  -c "targets" \
  -c "mdw 0xE0042000" \
  -c "mdw 0x1FFFF7E0" \
  -c "flash probe 0" \
  -c "echo {=== core is halted; SWD is yours. Leave this running or Ctrl-C and use make unlock/flash ===}" \
  -c "sleep 3600000"
