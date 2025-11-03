# boot/makefile

ESP_DIR := esp

KERNEL_SRC := ../kernel/build/kernel.elf
KERNEL_DEST := $(ESP_DIR)/kernel.elf
LIMINE_CONF := $(ESP_DIR)/limine.conf

IMG := image.hdd
IMG_SIZE := 256 # in MB

.PHONY: all esp image clean

all: image

esp: $(KERNEL_DEST)

$(KERNEL_DEST): $(KERNEL_SRC)
	@ echo "Copying kernel to ESP directory..."
	@ mkdir -p $(ESP_DIR)
	@ cp $< $@

image: $(IMG)

$(IMG): $(KERNEL_DEST) $(LIMINE_CONF)
	@ echo "Creating bootable image..."
	dd if=/dev/zero bs=1M count=0 seek=$(IMG_SIZE) of=$(IMG)
	sgdisk $(IMG) -n 1:2048 -t 1:ef00
	mformat -i $(IMG)@@1M
	mmd -i $(IMG)@@1M ::/EFI ::/EFI/BOOT
	mcopy -i $(IMG)@@1M $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $(IMG)@@1M $(LIMINE_CONF) ::/
	mcopy -i $(IMG)@@1M $(KERNEL_DEST) ::/

clean:
	@ rm -f $(KERNEL_DEST)
	@ rm -f $(IMG)
