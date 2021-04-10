mcu=attiny85
dev=t85
std=c++17

all: demo.elf

%.elf: %.cpp
	avr-g++ -std=$(std) -Os -mmcu=$(mcu) -flto -o $@ $< \
	-I../avrINT/include \
	-I../avrWDT/include \
	-I../avrIO/include \
	-I../avrSLEEP/include \
	-Iinclude
	avr-size $@

%.s: %.elf
	avr-objdump -d $< > $@
	cat $@

.PHONY: flash

flash-%: %.elf
	avrdude -p $(dev) -c usbasp -P usb flash -U flash:w:$<
