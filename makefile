##
## zero - pre-emptive multitasking kernel for AVR
##
## Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
## Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
##


##########################################################################################
########################### DEVELOPER-ADJUSTABLE SETTINGS HERE ###########################
##########################################################################################


# output file and project name
OUTPUT = zero

# flashing settings
AVRDUDE_PART = m328p
AVRDUDE_CFG = pi

# general kernel settings
F_CPU_MHZ = 16
QUANTUM_TICKS = 15
PAGE_BYTES = 16

# enabled drivers
ZERO_DRIVERS_SPIMEM = 1
ZERO_DRIVERS_USART = 1
ZERO_DRIVERS_SUART = 1
ZERO_DRIVERS_GPIO = 1
ZERO_DRIVERS_PIPE = 1

# comms
DEBUG_PIN = D1
DEBUG_BAUD = 9600

# some standard MCU settings
LFUSE = 0xFF
HFUSE = 0xD9
EFUSE = 0xFF

# All SRAM except 1K
DYNAMIC_BYTES = "((RAMEND - 255) - 1024)"

# MCU mappings
ifeq ($(AVRDUDE_PART),m328)
	# 1KB for allocator
	MCU = atmega328
	SPI_CFG = 1
endif

ifeq ($(AVRDUDE_PART),m328p)
	# 1KB for allocator
	MCU = atmega328p
	SPI_CFG = 1
endif

ifeq ($(AVRDUDE_PART),m644p)
	# 3KB for allocator
	MCU = atmega644p
	SPI_CFG = 2
endif

ifeq ($(AVRDUDE_PART),m1284p)
	# 15KB for allocator
	MCU = atmega1284p
	SPI_CFG = 2
endif


##########################################################################################
######################### END DEVELOPER-ADJUSTABLE SETTINGS HERE #########################
##########################################################################################


PC_COUNT = 2

# general flags
FLAGS += -g
FLAGS += -Os
FLAGS += --std=c++17
FLAGS += -mmcu=$(MCU)
FLAGS += -fshort-enums
FLAGS += -ffunction-sections
FLAGS += -fdata-sections
FLAGS += -Wl,-gc-sections

# paths
FLAGS += -Icore/
FLAGS += -Idrivers/
FLAGS += -Ihelpers/

# #define pass-throughs
FLAGS += -DF_CPU=$(F_CPU_MHZ)'000'000ULL
FLAGS += -DF_CPU_MHZ=$(F_CPU_MHZ)U
FLAGS += -DPC_COUNT=$(PC_COUNT)
FLAGS += -DPAGE_BYTES=$(PAGE_BYTES)
FLAGS += -DDYNAMIC_BYTES=$(DYNAMIC_BYTES)
FLAGS += -DPROJ_NAME=\"$(OUTPUT)\"
FLAGS += -DQUANTUM_TICKS=$(QUANTUM_TICKS)
FLAGS += -DSPI_CFG=$(SPI_CFG)

# warnings control
FLAGS += -Wall
FLAGS += -Wno-return-type

# drivers
ifeq ($(ZERO_DRIVERS_SPIMEM),1)
	FLAGS += -DZERO_DRIVERS_SPIMEM
endif

ifeq ($(ZERO_DRIVERS_USART),1)
	FLAGS += -DZERO_DRIVERS_USART
endif

ifeq ($(ZERO_DRIVERS_SUART),1)
	FLAGS += -DZERO_DRIVERS_SUART
endif

ifeq ($(ZERO_DRIVERS_GPIO),1)
	FLAGS += -DZERO_DRIVERS_GPIO
endif

ifeq ($(ZERO_DRIVERS_PIPE),1)
	FLAGS += -DZERO_DRIVERS_PIPE
endif

ifneq ($(DEBUG_PIN),)
	FLAGS += -DDEBUG_ENABLED
	FLAGS += -DDEBUG_PIN=ZERO_PIN$(DEBUG_PIN)
	FLAGS += -DDEBUG_BAUD=$(DEBUG_BAUD)
endif


# build targets
CC = avr-g++
SRC := $(wildcard *.cpp)
SRC += $(wildcard core/*.cpp)
SRC += $(wildcard drivers/*.cpp)
SRC += $(wildcard helpers/*.cpp)
SRC += $(wildcard cli/*.cpp)


.PHONY: push fuses upload clean gettools


$(OUTPUT).elf: $(SRC)
	@echo -n "Building..."
	@$(CC) $(FLAGS) -o $@ $^
	@echo " done"
	@avr-size -A -x --mcu=$(MCU) $@
	@avr-size -C -x --mcu=$(MCU) $@


upload: $(OUTPUT).elf
	@sudo avrdude -p $(AVRDUDE_PART) -c $(AVRDUDE_CFG) -U flash:w:$(OUTPUT).elf


fuses:
	@sudo avrdude -p $(AVRDUDE_PART) -c $(AVRDUDE_CFG) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m


push: $(OUTPUT).elf
	@echo -n "Pushing files to RPi..."
	@rsync -avr --exclude '.git' ~/dev/$(OUTPUT)/ pi:dev/$(OUTPUT)/ > /dev/null
	@rm -f *.o *.elf *.hex
	@echo " done"


clean:
	@echo -n "Cleaning up..."
	@rm -f *.o *.elf *.hex
	@echo " done"


gettools:
	@sudo apt-get -y install gcc-avr binutils-avr gdb-avr avr-libc avrdude
