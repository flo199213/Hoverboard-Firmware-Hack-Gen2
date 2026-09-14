#!/usr/bin/env bash
# Sanity-check the vector table of both firmware images WITHOUT any hardware.
#
# For each build-<board>/hoverboard-<board>.bin:
#   word0 (initial SP)     must be 0x20002000  (top of the 8 KiB SRAM of GD32F130C8)
#   word1 (reset vector)   must be odd (Thumb bit) and point inside
#                          0x08000000..0x0800FFFF (64 KiB flash), and inside the image
#   image size             must fit in 64 KiB
# If the matching .elf is present, the reset vector is cross-checked against
# the ELF entry point (arm-none-eabi-readelf) and Reset_Handler (objdump).
#
# Usage:  tools/check-bins.sh [bin ...]     (defaults to both boards)
# Exit status is non-zero if any check fails.
set -u
cd "$(dirname "$0")/.."

RAM_TOP=$((0x20002000))
FLASH_BASE=$((0x08000000))
FLASH_SIZE=$((64 * 1024))

bins=("$@")
if [[ ${#bins[@]} -eq 0 ]]; then
  bins=(build-master/hoverboard-master.bin build-slave/hoverboard-slave.bin)
fi

fail=0
note() { printf '   %-8s %s\n' "$1" "$2"; }
bad()  { note "FAIL" "$1"; fail=1; }
good() { note "ok" "$1"; }

for bin in "${bins[@]}"; do
  echo "== $bin"
  if [[ ! -f "$bin" ]]; then bad "file missing (run make BOARD=... first)"; continue; fi

  size=$(stat -c %s "$bin")
  read -r w0 w1 < <(od -An -tx4 -N8 -v "$bin")
  sp=$((16#$w0)); rv=$((16#$w1))
  printf '   word0 (SP)    = 0x%08x\n   word1 (reset) = 0x%08x\n   size          = %d bytes\n' "$sp" "$rv" "$size"

  if (( sp == RAM_TOP )); then good "initial SP is top of 8 KiB SRAM"; else bad "initial SP != 0x20002000"; fi
  if (( rv & 1 )); then good "reset vector has Thumb bit set"; else bad "reset vector is even (no Thumb bit)"; fi
  addr=$(( rv & ~1 ))
  if (( addr >= FLASH_BASE && addr < FLASH_BASE + FLASH_SIZE )); then good "reset vector inside 64 KiB flash"; else bad "reset vector outside flash"; fi
  if (( addr - FLASH_BASE < size )); then good "reset vector inside the image"; else bad "reset vector beyond end of image"; fi
  if (( size <= FLASH_SIZE )); then good "image fits in 64 KiB flash"; else bad "image larger than flash"; fi

  elf="${bin%.bin}.elf"
  if [[ -f "$elf" ]] && command -v arm-none-eabi-readelf >/dev/null; then
    entry=$(arm-none-eabi-readelf -h "$elf" | awk '/Entry point/ {print $NF}')
    if (( entry == rv )); then good "matches ELF entry point $entry"; else bad "ELF entry point $entry != reset vector"; fi
    rh=$(arm-none-eabi-objdump -t "$elf" | awk '$NF=="Reset_Handler" {print $1; exit}')
    if [[ -n "$rh" ]] && (( 16#$rh == addr )); then good "matches Reset_Handler symbol 0x$rh"; fi
    vt=$(arm-none-eabi-objdump -h "$elf" | awk '$2==".isr_vector" {print $4}')
    if [[ -n "$vt" ]] && (( 16#$vt == FLASH_BASE )); then good ".isr_vector placed at 0x08000000"; else bad ".isr_vector not at 0x08000000 (got ${vt:-none})"; fi
  fi
done

if (( fail )); then echo "RESULT: FAIL"; exit 1; fi
echo "RESULT: all checks passed"
