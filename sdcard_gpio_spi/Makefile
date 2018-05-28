COMMON = ../project_common
ASRCS = boot.S
SRCS1 = kernel.c sdcard.c file.c helper.c
SRCS2 = $(COMMON)/uart.c $(COMMON)/gpio.c
SRCS3 = $(COMMON)/mylib.c
AOBJS := $(ASRCS:%.S=%.o)
OBJS1 := $(SRCS1:%.c=%.o)
OBJS2 := $(SRCS2:$(COMMON)/%.c=%.o)
OBJS3 := $(SRCS3:$(COMMON)/%.c=%.o)
OBJS := $(AOBJS) $(OBJS1) $(OBJS2) $(OBJS3)
ARCH = armv6zk
ISA = ARM6v2
CPU = arm1176jzf-s
CC = arm-none-eabi-gcc
CFLAG =  -O0 -g -gdwarf-2 -march=$(ARCH) -mtune=$(CPU) -s -nostdlib -nostartfiles -ffreestanding -c -fno-builtin -DDEBUG
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
	$(LD) -T linker.ld $(OBJS) -o myos.elf $(LFLAG)

$(AOBJS): %.o: %.S
	$(ASM) -c $< -o $@

# source codes unique to this folder
$(OBJS1): %.o: %.c
	$(CC) $(CFLAG) $(CFLAG2) $(INCLUDE) $< -o $@

# source codes common to other projects
$(OBJS2): %.o: $(COMMON)/%.c
	$(CC) $(CFLAG) $(CFLAG2) $(INCLUDE) $< -o $@

# source codes that doesn't work with gnu99 option
$(OBJS3): %.o: $(COMMON)/%.c
	$(CC) $(CFLAG) $(INCLUDE) $< -o $@

.PHONY:clean
clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.img
	rm -f *~
