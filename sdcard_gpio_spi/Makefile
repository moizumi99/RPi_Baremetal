COMMON = ../project_common
OBJS = boot.o kernel.o sdcard.o file.o helper.o $(COMMON)/uart.o $(COMMON)/mylib.o $(COMMON)/gpio.o
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

all: kernel.img

kernel.img: myos.elf
	arm-none-eabi-objcopy myos.elf -O binary kernel.img

myos.elf: $(OBJS) linker.ld
	make -C $(COMMON)
	$(LD) -T linker.ld $(OBJS) -o myos.elf $(LFLAG)

.S.o:
	$(ASM) -c $< -o $@

mylib.o: mylib.c
	$(CC) $(CFLAG) $(INCLUDE) mylib.c -o mylib.o

# helper_test: helper_test.c helper.c helper.h
# 	$(TESTCC) helper_test.c helper.c -o helper_test $(TESTFLAG)

.c.o:
	$(CC) $(CFLAG) $(CFLAG2) $(INCLUDE) $< -o $@ 

.PHONY:clean
clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.img
	rm -f *~
