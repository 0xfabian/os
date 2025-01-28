IMG_PATH = C:/Users/spide/Desktop/os
IMG = $(shell wslpath -a $(IMG_PATH))/image.hdd
QEMU_FLAGS = -cpu qemu64 -m 1G -net none \
	-drive format=raw,file=$(IMG_PATH)/image.hdd \
	-drive if=pflash,format=raw,unit=0,file=$(IMG_PATH)/OVMFbin/OVMF_CODE-pure-efi.fd,readonly=on \
	-drive if=pflash,format=raw,unit=1,file=$(IMG_PATH)/OVMFbin/OVMF_VARS-pure-efi.fd \

CC = gcc
ASMC = nasm
LD = ld
LDSCRIPT = link.ld

CFLAGS = -ffreestanding -mno-red-zone -Iinclude -O2
ASMFLAGS = 
LDFLAGS = -T $(LDSCRIPT) -static -Bsymbolic -nostdlib

SRCDIR := src
OBJDIR := build

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.cpp)
ASMSRC = $(call rwildcard,$(SRCDIR),*.asm)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(ASMSRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel: esp/boot/kernel

esp/boot/kernel: $(OBJS) $(LDSCRIPT)
	@ echo ---------- LINKING ----------
	$(LD) $(LDFLAGS) -o esp/boot/kernel $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo ------- COMPILING C++ ------- $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@ echo ------- COMPILING ASM ------- $^
	@ mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -f elf64 -o $@

buildimg:
	@ echo ---------- BUILDING IMAGE ----------
	dd if=/dev/zero bs=1M count=0 seek=256 of=image.hdd
	sgdisk image.hdd -n 1:2048 -t 1:ef00
	mformat -i image.hdd@@1M
	mmd -i image.hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i image.hdd@@1M esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i image.hdd@@1M esp/boot/limine/limine.conf ::/boot/limine
	mcopy -i image.hdd@@1M esp/boot/kernel ::/boot
	mv image.hdd $(IMG)

updateimg: $(IMG)

$(IMG): esp/boot/kernel
	@ echo ---------- UPDATING IMAGE ----------
	mcopy -o -i $(IMG)@@1M esp/boot/kernel ::/boot

clean:
	@ echo ---------- CLEANING ----------
	rm -rf $(OBJDIR)

run:
	@ powershell.exe -Command "qemu-system-x86_64 $(QEMU_FLAGS)"

all: kernel updateimg run