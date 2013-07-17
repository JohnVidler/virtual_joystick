CC=gcc
CFLAGS=-std=gnu99

.PHONY: clean

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all: VirtualJoystick

VirtualJoystick: VirtualJoystick.o SerialPort.o
	$(CC) VirtualJoystick.o SerialPort.o -o VirtualJoystick

clean:
	rm -f *.o
	rm VirtualJoystick
