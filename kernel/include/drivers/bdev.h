#pragma once

#include <types.h>

struct BlockDevice
{
    u32 block_size;

    bool init();
    bool read(u64 offset, u8* buf, usize count);
};

extern BlockDevice ata_bdev;
