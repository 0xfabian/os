
SRC = $(wildcard src/*)

lib:
	nasm -f elf64 -o sys/sys.o sys/sys.asm

utils:
	@ mv sys/stdarg.h sys/stdarg.h.skip
	@ for i in $(SRC); do \
		echo "gcc   $$i"; \
		base=`basename $$i | sed -E 's/\..*//'`; \
		if [ $$base != "cow" ]; then \
			gcc $$i sys/sys.o -o bin/$$base -Isys -O2 -static -nostdlib -fno-builtin -mgeneral-regs-only -fno-stack-protector -fno-exceptions; \
		else \
			gcc $$i sys/sys.o -o bin/$$base -Isys -O2 -static -nostdlib -fno-builtin -fno-stack-protector -fno-exceptions -ffast-math; \
		fi \
	done
	@ mv sys/stdarg.h.skip sys/stdarg.h

move:
	sudo cp -r bin ../disk

clean:
	rm bin/*

all: lib utils move
