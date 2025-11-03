# kernel/makefile

BUILD_DIR := build
OBJS_DIR := $(BUILD_DIR)/objs
TARGET := $(BUILD_DIR)/kernel.elf

CC = g++
AS = nasm
LD = ld
LD_SCRIPT = link.ld

CC_FLAGS = -Iinclude -O2 -Wall -mno-red-zone -mgeneral-regs-only -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector
AS_FLAGS = -f elf64
LD_FLAGS = -T $(LD_SCRIPT) -static -Bsymbolic -nostdlib

SRC_CPP = $(shell find src -name '*.cpp')
SRC_ASM = $(shell find src -name '*.asm')

OBJS = $(patsubst src/%.cpp,$(OBJS_DIR)/%.o,$(SRC_CPP)) \
	   $(patsubst src/%.asm,$(OBJS_DIR)/%_asm.o,$(SRC_ASM))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) $(LD_SCRIPT)
	@ echo linking kernel...
	@ $(LD) $(LD_FLAGS) -o $(TARGET) $(OBJS)

$(OBJS_DIR)/%.o: src/%.cpp
	@ echo '$(CC)   ' $^
	@ mkdir -p $(@D)
	@ $(CC) $(CC_FLAGS) -c $^ -o $@

$(OBJS_DIR)/%_asm.o: src/%.asm
	@ echo '$(AS)  ' $^
	@ mkdir -p $(@D)
	@ $(AS) $(AS_FLAGS) $^ -o $@

clean:
	@ rm -rf $(BUILD_DIR)
