#!/usr/bin/env bash
# Build both boards (master + slave) with the Linux/GCC Makefile, then print
# ELF section sizes, .bin sizes and sha256 sums, and run the vector-table check.
# Touches no hardware.
#
# Usage:  tools/build-all.sh [--clean]
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ "${1:-}" == "--clean" ]]; then
  make clean
fi

for board in master slave; do
  echo "== make BOARD=$board"
  if ! out=$(make BOARD="$board" 2>&1); then
    echo "$out"
    echo "BUILD FAILED for BOARD=$board" >&2
    exit 1
  fi
  echo "$out" | grep -E '^(\s+[0-9]+\s+[0-9]+\s+[0-9]+|\s+text)' || true
done

echo
echo "== ELF sizes"
arm-none-eabi-size build-master/hoverboard-master.elf build-slave/hoverboard-slave.elf

echo
echo "== .bin files"
ls -l build-master/hoverboard-master.bin build-slave/hoverboard-slave.bin

echo
echo "== sha256"
sha256sum build-master/hoverboard-master.bin build-slave/hoverboard-slave.bin

echo
exec "$(dirname "$0")/check-bins.sh"
