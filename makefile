CC = g++
AS = nasm
LD = ld
LD_SCRIPT = link.ld

CC_FLAGS = -Iinclude -O2 -Wall -mno-red-zone -mgeneral-regs-only -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector
AS_FLAGS = -f elf64
LD_FLAGS = -T $(LD_SCRIPT) -static -Bsymbolic -nostdlib

QEMU_FLAGS = -display sdl -enable-kvm -cpu host -m 1G -net none \
	-drive format=raw,file=image.hdd \
	-drive if=pflash,format=raw,readonly=on,file=ovmf/OVMF_CODE.4m.fd \
	-drive if=pflash,format=raw,file=ovmf/OVMF_VARS.4m.fd \
	-drive if=ide,format=raw,file=disk.img

SRCDIR := src
OBJDIR := build

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.cpp)
ASM_SRC = $(call rwildcard,$(SRCDIR),*.asm)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(ASM_SRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel: esp/boot/kernel

esp/boot/kernel: $(OBJS) $(LD_SCRIPT)
	@ echo linking kernel...
	@ $(LD) $(LD_FLAGS) -o esp/boot/kernel $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo 'g++   ' $^
	@ mkdir -p $(@D)
	@ $(CC) $(CC_FLAGS) -c $^ -o $@

$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@ echo 'nasm  ' $^
	@ mkdir -p $(@D)
	@ $(AS) $(AS_FLAGS) $^ -o $@

buildimg:
	dd if=/dev/zero bs=1M count=0 seek=256 of=image.hdd
	sgdisk image.hdd -n 1:2048 -t 1:ef00
	mformat -i image.hdd@@1M
	mmd -i image.hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i image.hdd@@1M esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i image.hdd@@1M esp/boot/limine/limine.conf ::/boot/limine
	mcopy -i image.hdd@@1M esp/boot/kernel ::/boot

updateimg: esp/boot/kernel
	@ echo updating image...
	@ mcopy -o -i image.hdd@@1M esp/boot/kernel ::/boot

clean:
	@ rm -rf $(OBJDIR)

run:
	@ sudo umount disk 2> /dev/null || true
	@ qemu-system-x86_64 $(QEMU_FLAGS) 2> /dev/null
	@ sudo mount -o loop disk.img disk

all: kernel updateimg run