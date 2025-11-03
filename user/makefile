# user/makefile

SYS := lib/sys.o
SYS_SRC := src/sys.asm

SSE_BIN := cow rt
BASE_FLAGS := -Iinclude -O2 -static -nostdlib -fno-builtin -fno-stack-protector -fno-exceptions
REGULAR_FLAGS := $(BASE_FLAGS) -mgeneral-regs-only
SEE_FLAGS := $(BASE_FLAGS) -ffast-math

BINS := $(patsubst src/%.c,bin/%, $(shell find src -name '*.c')) \
		$(patsubst src/%.cpp,bin/%, $(shell find src -name '*.cpp'))

.PHONY: all sys utils clean

all: sys utils

sys: $(SYS)

$(SYS): $(SYS_SRC)
	@ echo 'nasm   ' $^
	@ mkdir -p lib
	@ nasm -f elf64 -o $(SYS) $(SYS_SRC)

utils: $(BINS)

bin/%: src/%.c $(SYS)
	@ mkdir -p bin
	@ echo "gcc   " $<
	@ if echo "$(SSE_BIN)" | grep -wq `basename $@`; then \
		gcc $< $(SYS) -o $@ $(SEE_FLAGS); \
	else \
		gcc $< $(SYS) -o $@ $(REGULAR_FLAGS); \
	fi

bin/%: src/%.cpp $(SYS)
	@ mkdir -p bin
	@ echo "g++   " $<
	@ g++ $< $(SYS) -o $@ $(REGULAR_FLAGS)

clean:
	rm -rf bin
	rm -rf lib
