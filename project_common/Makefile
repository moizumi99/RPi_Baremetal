ARCH = armv6zk
ISA = ARM6v2
CPU = arm1176jzf-s
CC = arm-none-eabi-gcc
CFLAG =  -O0 -g -gdwarf-2 -march=$(ARCH) -mtune=$(CPU) -s -nostdlib -nostartfiles -ffreestanding -c -fno-builtin
CFLAG2 =  -std=gnu99
INCLUDE = -I $(COMMON)
ASM = arm-none-eabi-gcc
AFLAG = -mcpu=$(CPU) -ffreestanding -c -fpic
LD = arm-none-eabi-gcc
LIB = -L /usr/lib/gcc/arm-none-eabi/4.9.3/ -L. -lgcc
LFLAG = -static -nostdlib $(LIB) -Wl,-Map,kernel.map
TESTCC = gcc
TESTFLAG = -lcunit

all: mylib.o uart.o gpio.o

mylib.o: mylib.c
	$(CC) $(CFLAG) mylib.c -o mylib.o

.S.o:
	$(ASM) -c $< -o $@

.c.o:
	$(CC) $(CFLAG) $(CFLAG2) $< -o $@ 

.PHONY:clean
clean:
	rm -f *.o
	rm -f *~
