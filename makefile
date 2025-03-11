WIN_PATH = C:/Users/spide/Desktop/os
IMG = $(shell wslpath -a $(WIN_PATH))/image.hdd
DISK = $(shell wslpath -a $(WIN_PATH))/disk.img
QEMU_FLAGS = -cpu qemu64 -m 1G -net none \
	-drive format=raw,file=$(WIN_PATH)/image.hdd \
	-drive if=pflash,format=raw,unit=0,file=$(WIN_PATH)/OVMFbin/OVMF_CODE-pure-efi.fd,readonly=on \
	-drive if=pflash,format=raw,unit=1,file=$(WIN_PATH)/OVMFbin/OVMF_VARS-pure-efi.fd \
	-drive if=ide,format=raw,file=$(WIN_PATH)/disk.img

CC = g++
ASMC = nasm
LD = ld
LDSCRIPT = link.ld

CFLAGS = -Iinclude -O2 -Wall -mno-red-zone -mgeneral-regs-only -ffreestanding -fno-exceptions -fno-rtti
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
	@ echo linking kernel...
	@ $(LD) $(LDFLAGS) -o esp/boot/kernel $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo 'g++   ' $^
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@ echo 'nasm  ' $^
	@ mkdir -p $(@D)
	@ $(ASMC) $(ASMFLAGS) $^ -f elf64 -o $@

buildimg:
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
	@ echo updating image...
	@ mcopy -o -i $(IMG)@@1M esp/boot/kernel ::/boot

clean:
	@ rm -rf $(OBJDIR)

run:
	@ sudo umount disk 2> /dev/null || true
	@ powershell.exe -Command "qemu-system-x86_64 $(QEMU_FLAGS)" 2> /dev/null
	@ sudo mount -o loop $(DISK) disk

all: kernel updateimg run