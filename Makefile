CROSS=arm-none-eabi-
CC=$(CROSS)gcc
LD=$(CROSS)ld
AS=$(CROSS)as
OBJCOPY=$(CROSS)objcopy

CFLAGS = -nostdlib -nostartfiles -ffreestanding -O2 -Wall -Wextra -march=armv4t
LDFLAGS = -T linker.ld -nostdlib

OBJS = boot.o kernel.o

all: kernel.img

boot.o: boot.s
	$(AS) -o $@ $<

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

kernel.img: kernel.elf
	$(OBJCOPY) -O binary kernel.elf kernel.img

clean:
	rm -f *.o *.elf *.img
