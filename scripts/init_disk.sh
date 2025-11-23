#!/bin/bash

# This creates a disk image file formatted with ext2 filesystem
# and mounts it to a directory.

set -e

DISK_FILE="disk.img"
DISK_SIZE=512  # MB
DISK_DIR="disk"

if [ ! -f "$DISK_FILE" ]; then
    dd if=/dev/zero of="$DISK_FILE" bs=1M count=$DISK_SIZE &> /dev/null
    mkfs.ext2 -I 1024 "$DISK_FILE" &> /dev/null
    echo "Created disk image '$DISK_FILE' with size ${DISK_SIZE}MB."
else
    echo "Skipping creation: '$DISK_FILE' already exists."
fi

mkdir -p "$DISK_DIR"

if mount | grep -q "$DISK_DIR"; then
    echo "Skipping mount: '$DISK_DIR' is already mounted."
else
    sudo mount -o loop "$DISK_FILE" "$DISK_DIR"
    sudo chown $USER:$USER "$DISK_DIR"
    echo "Mounted '$DISK_FILE' at '$DISK_DIR'."
fi
