# GNU Make build for Hoverboard-Firmware-Hack-Gen2 (GD32F130C8, Cortex-M3)
# Linux/GCC port of the Keil project. Usage:
#   make BOARD=master      (default)
#   make BOARD=slave
#   make flash BOARD=...   (openocd, ST-Link V2)
#   make unlock            (clear flash read-protection on a virgin board)
#   make clean

BOARD ?= master
BUILD_DIR = build-$(BOARD)
TARGET = hoverboard-$(BOARD)

ifeq ($(BOARD),master)
BOARD_DEF = -DMASTER
else ifeq ($(BOARD),slave)
BOARD_DEF = -DSLAVE
else
$(error BOARD must be 'master' or 'slave')
endif

OPT = -Os
DEBUG = 1

DRIVERS_DIR = RTE/Device/GD32F130C8
C_SOURCES = \
$(DRIVERS_DIR)/gd32f1x0_adc.c \
$(DRIVERS_DIR)/gd32f1x0_dbg.c \
$(DRIVERS_DIR)/gd32f1x0_dma.c \
$(DRIVERS_DIR)/gd32f1x0_fwdgt.c \
$(DRIVERS_DIR)/gd32f1x0_gpio.c \
$(DRIVERS_DIR)/gd32f1x0_i2c.c \
$(DRIVERS_DIR)/gd32f1x0_misc.c \
$(DRIVERS_DIR)/gd32f1x0_opa.c \
$(DRIVERS_DIR)/gd32f1x0_pmu.c \
$(DRIVERS_DIR)/gd32f1x0_rcu.c \
$(DRIVERS_DIR)/gd32f1x0_syscfg.c \
$(DRIVERS_DIR)/gd32f1x0_timer.c \
$(DRIVERS_DIR)/gd32f1x0_usart.c \
$(DRIVERS_DIR)/gd32f1x0_wwdgt.c \
$(DRIVERS_DIR)/system_gd32f1x0.c \
Src/main.c \
Src/it.c \
Src/setup.c \
Src/bldc.c \
Src/led.c \
Src/comms.c \
Src/commsMasterSlave.c \
Src/commsSteering.c \
Src/commsBluetooth.c

ASM_SOURCES = startup_gd32f1x0.s

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

MCU = -mcpu=cortex-m3 -mthumb
C_DEFS = -DUSE_STDPERIPH_DRIVER -DGD32F130_150 $(BOARD_DEF)
C_INCLUDES = -IInc -IRTE/Include -IRTE/_Target_1

ASFLAGS = $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

LDSCRIPT = GD32F130C8_FLASH.ld
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -specs=nosys.specs -T$(LDSCRIPT) $(LIBS) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir -p $@

# ---- ST-Link / OpenOCD targets ------------------------------------------
# GD32F130 is a Cortex-M3 with an STM32F1-compatible flash controller, so
# OpenOCD's stm32f1x driver works. Hold the board's power button during
# every flash operation — the firmware releases the power latch otherwise.
OPENOCD = openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "reset_config none"

probe:
	$(OPENOCD) -c "init; halt; targets; mdw 0xE0042000; mdw 0x4002201C; shutdown"

# Clears read-protection (RDP). This MASS-ERASES the chip — irreversible.
# The chip must then be power-cycled for the new option bytes to load.
unlock:
	$(OPENOCD) -c "init; halt; stm32f1x unlock 0; shutdown"
	@echo ">>> RDP cleared + mass erase done. POWER-CYCLE the board now, then run: make verify-unlocked"

verify-unlocked:
	$(OPENOCD) -c "init; halt; mdw 0x4002201C; mdw 0x08000000 4; shutdown"
	@echo ">>> expect FLASH_OBR bit1 = 0 (e.g. ...ff00) and flash reading as ffffffff"

flash: $(BUILD_DIR)/$(TARGET).bin
	$(OPENOCD) -c "init; halt; program $(BUILD_DIR)/$(TARGET).bin 0x08000000 verify; reset run; shutdown"

clean:
	-rm -rf build-master build-slave

-include $(wildcard $(BUILD_DIR)/*.d)
.PHONY: all clean probe unlock verify-unlocked flash
