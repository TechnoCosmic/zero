# zero - pre-emptive multitasking kernel for AVR
#
#  Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
#  Catchpole Robotics					Christian Catchpole		christian@catchpole.net

# adjust these to suit your needs

OUTPUT = demo
AVRDUDE_PART = m328p
AVRDUDE_CFG = pi
F_CPU = 16000000UL


# some standard MCU settings

LFUSE = 0xFF
HFUSE = 0xD9
EFUSE = 0xFF


# the default HEAP_HEX_END values give 1K
# to the globals/kernel stack, and the
# remainder to the dynamic allocator.
# change these as you need

# memory for allocator = HEAP_END_HEX - 0x100 (HEAP_START_HEX)
HEAP_START_HEX = 0100


ifeq ($(AVRDUDE_PART),m328p)
	MCU = atmega328p
	HEAP_END_HEX = 0700
endif

ifeq ($(AVRDUDE_PART),m644p)
	MCU = atmega644p
	HEAP_END_HEX = 1E00
endif

ifeq ($(AVRDUDE_PART),m1284p)
	MCU = atmega1284p
	HEAP_END_HEX = 3E00
endif

ifeq ($(AVRDUDE_PART),m2560)
	MCU = atmega2560
	HEAP_END_HEX = 1E00
endif


# probably don't adjust these so much :)
FLAGS += -Os
FLAGS += -g
FLAGS += --std=c++17
FLAGS += -mmcu=$(MCU)
FLAGS += -DF_CPU=$(F_CPU)
FLAGS += -DDYNAMIC_BYTES="(0x$(HEAP_END_HEX)-0x$(HEAP_START_HEX))"
FLAGS += -DPROJ_NAME=\"$(OUTPUT)\"

CC = avr-gcc
OBJ := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LDFLAGS := -Wl,--section-start=.heap=0x80$(HEAP_START_HEX),--section-start=.data=0x80$(HEAP_END_HEX)


.PHONY: push fuses upload clean gettools


%.o: %.cpp
	@echo -n "Compiling $^..."
	@$(CC) $(FLAGS) -c $^
	@echo " done"


$(OUTPUT).hex: $(OBJ)
	@echo -n "Linking..."
	@$(CC) $(FLAGS) $(LDFLAGS) -o $(OUTPUT).elf $^
	@echo " done"
	@avr-objcopy -j .text -j .data -O ihex $(OUTPUT).elf $(OUTPUT).hex
	@avr-size -C --mcu=$(MCU) $(OUTPUT).elf
	@rm -f *.elf *.gch


upload: $(OUTPUT).hex
	@sudo avrdude -p $(AVRDUDE_PART) -c $(AVRDUDE_CFG) -U flash:w:$(OUTPUT).hex


fuses:
	@sudo avrdude -p $(AVRDUDE_PART) -c $(AVRDUDE_CFG) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m


push: $(OUTPUT).hex
	@echo -n "Pushing files to RPi..."
	@rsync -avr --exclude '.git' ~/dev/zero/ pi:dev/zero/ > /dev/null
	@rm -f *.o *.elf *.hex *.gch
	@echo " done"


clean:
	@echo -n "Cleaning up..."
	@rm -f *.o *.elf *.hex *.gch
	@echo " done"


gettools:
	@sudo apt-get -y install gcc-avr binutils-avr gdb-avr avr-libc avrdude
