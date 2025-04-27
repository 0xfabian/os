# gcc src/ed.cpp sys/sys.o -o bin/ed -Isys -O2 -static -nostdlib -fno-builtin -fno-stack-protector -fno-exceptions -fno-rtti -mgeneral-regs-only

SRC = $(wildcard src/*)

lib:
	nasm -f elf64 -o sys/sys.o sys/sys.asm

utils:
	@ for i in $(SRC); do \
		echo "gcc   $$i"; \
		gcc $$i sys/sys.o -o bin/`basename $$i | sed -E 's/\..*//'` -Isys -O2 -static -nostdlib -fno-builtin -mgeneral-regs-only -fno-stack-protector -fno-exceptions; \
	done

move:
	sudo cp -r bin ../disk

clean:
	rm bin/*

all: lib utils move