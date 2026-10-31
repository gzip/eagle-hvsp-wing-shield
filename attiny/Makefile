# Microcontroller configuration
MCU      = attiny85
F_CPU    = 1000000UL
TARGET   = main
SOURCES  = $(TARGET).c

# Programmer settings (override with: make flash PROGRAMMER=arduino PORT=COM3 BITCLOCK=32)
PROGRAMMER ?= usbasp
PORT       ?=
BAUD       ?=
BITCLOCK   ?= $(if $(filter usbasp,$(PROGRAMMER)),32,)

# Toolchain commands
CC       ?= avr-gcc
OBJCOPY  ?= avr-objcopy
OBJDUMP  ?= avr-objdump
SIZE     ?= avr-size
AVRDUDE  ?= avrdude
RM       ?= rm -f

# Compiler & Linker flags
CFLAGS   = -Wall -Wextra -Wno-array-bounds -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)
LDFLAGS  = -mmcu=$(MCU)

# Avrdude flags
AVRDUDE_FLAGS = -c $(PROGRAMMER) -p t85
ifneq ($(BITCLOCK),)
AVRDUDE_FLAGS += -B $(BITCLOCK)
endif
ifneq ($(PORT),)
AVRDUDE_FLAGS += -P $(PORT)
endif
ifneq ($(BAUD),)
AVRDUDE_FLAGS += -b $(BAUD)
endif

# Default target
all: $(TARGET).hex size

$(TARGET).o: $(SOURCES)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(TARGET).o
	$(CC) $(LDFLAGS) $< -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

size: $(TARGET).elf
	@echo ""
	$(SIZE) --format=avr --mcu=$(MCU) $<

flash: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i

clean:
	$(RM) $(TARGET).o $(TARGET).elf $(TARGET).hex

.PHONY: all size flash clean
