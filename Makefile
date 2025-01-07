MCU = atmega328p
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
PROGRAMMER = usbtiny

CFLAGS = -Os -mmcu=$(MCU)
LDFLAGS = -mmcu=$(MCU)

TARGET = main

default: bin/$(TARGET).hex

bin/$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c -o $@ $<

bin/$(TARGET).elf: bin/$(TARGET).o
	$(CC) $(LDFLAGS) -o $@ $^

bin/$(TARGET).hex: bin/$(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

upload: bin/$(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -B 5 -U flash:w:$<:i -V

clean:
	rm -f bin/*.o bin/*.elf bin/*.hex
 
.PHONY: default upload clean
