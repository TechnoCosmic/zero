# zero - pre-emptive multitasking kernel for AVR
#
#  Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
#  Catchpole Robotics					Christian Catchpole		christian@catchpole.net

# adjust these to suit your needs

# NOTE: If you adjust the MCU, then be sure you update
# the fuses section of this makefile - you don't want
# to brick an MCU because the fuses weren't right!!!
OUTPUT=zero
AVRDUDE_CFG=pi
AVRDUDE_PART=m1284p
MCU=atmega1284p
F_CPU=20000000UL

# probably don't adjust these so much :)
CC=avr-gcc
FLAGS +=-Os
FLAGS +=-g
FLAGS +=--std=c++17
FLAGS +=-mmcu=$(MCU)
FLAGS +=-DF_CPU=$(F_CPU)

OBJ:=$(patsubst %.cpp,%.o,$(wildcard *.cpp))


.PHONY: push fuses upload clean gettools


%.o: %.cpp
	@echo -n "Compiling $^..."
	@$(CC) $(FLAGS) -c $^
	@echo " done"


$(OUTPUT).hex: $(OBJ)
	@echo -n "Linking..."
	@$(CC) $(FLAGS) -o $(OUTPUT).elf $^
	@echo " done"
	@avr-objcopy -j .text -j .data -O ihex $(OUTPUT).elf $(OUTPUT).hex
	@avr-size -C --mcu=$(MCU) $(OUTPUT).elf
	@rm -f *.elf *.gch


upload: $(OUTPUT).hex
	@sudo avrdude -p $(AVRDUDE_PART) -c $(AVRDUDE_CFG) -U flash:w:$(OUTPUT).hex


# these fuses are verified correct for 328P, 644P, and 1284P
fuses:
	@sudo avrdude -p $(AVRDUDE_PART) -c $(AVRDUDE_CFG) -U lfuse:w:0xff:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m


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
