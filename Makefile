CC=avr-gcc
CFLAGS=-g -Os -mcall-prologues -mmcu=atmega8 -DF_CPU=12000000
PROZ=m8
OBJ2HEX=avr-objcopy 
UISP=avrdude
OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o

all: main.hex

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.S.o:
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(CC) $(CFLAGS) -S $< -o $@

flash: all
	$(UISP) -c usbasp -p m8 -U flash:w:main.hex:i

fuse:
	$(UISP) -B 100 -c usbasp -p $(PROZ) -U hfuse:w:0xc9:m
	$(UISP) -B 100 -c usbasp -p $(PROZ) -U lfuse:w:0x9f:m

clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.bin *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

main.bin: $(OBJECTS)
	$(CC) $(CFLAGS) -o main.bin $(OBJECTS)

main.hex: main.bin
	rm -f main.hex main.eep.hex
	$(OBJ2HEX) -j .text -j .data -O ihex main.bin main.hex

cpp:
	$(CC) $(CFLAGS) -E main.c

prog: all
	$(UISP) -c usbasp -p $(PROZ) -B 60 -U flash:w:main.hex:i 
