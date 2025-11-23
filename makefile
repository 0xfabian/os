# Top level makefile

IMG := boot/image.hdd
DISK := disk.img

OVMF_CODE := ovmf/OVMF_CODE.fd
OVMF_VARS := ovmf/OVMF_VARS.fd

.PHONY: all kernel boot user clean run help

all: kernel boot

kernel:
	@ $(MAKE) -C kernel

boot:
	@ $(MAKE) -C boot

user:
	@ $(MAKE) -C user

clean:
	@ $(MAKE) -C kernel clean
	@ $(MAKE) -C boot clean
	@ $(MAKE) -C user clean

run:
	@ qemu-system-x86_64 -cpu qemu64 -m 1G -net none \
		-drive format=raw,file=$(IMG) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS) \
		$$( [ -f $(DISK) ] && echo "-drive if=ide,format=raw,file=$(DISK)" )
