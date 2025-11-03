#include <drivers/bdev.h>
#include <drivers/ata.h>

ATA* ata;
BlockDevice ata_bdev;

// this is just a hack
// the kernel must deal with devices in a more organized way

bool BlockDevice::init()
{
    ata = ATA::from(0x1f0, false);

    if (!ata->identify())
    {
        kfree(ata);
        ata = nullptr;

        return false;
    }

    return true;
}

bool BlockDevice::read(u64 offset, u8* buf, usize count)
{
    // kprintf("BlockDevice::read\n");

    if (!ata)
        return false;

    u64 lba = offset / 512;
    u32 sector_offset = offset % 512;
    u64 end = offset + count;
    u8 temp[512];

    usize buf_offset = 0;

    while (offset < end)
    {
        if (!ata->read(lba, temp, 512))
            return false;

        // kprintf("BlockDevice::read: read sector %d\n", lba);

        usize copy_start = (offset != (lba * 512)) ? sector_offset : 0;
        usize copy_length = 512 - copy_start;

        if (count - buf_offset < copy_length)
            copy_length = count - buf_offset;

        memcpy(buf + buf_offset, temp + copy_start, copy_length);

        buf_offset += copy_length;
        offset += copy_length;
        lba++;
    }

    // kprintf("BlockDevice::read: read %d bytes\n", count);

    // hexdump(buf, count);

    return true;
}
