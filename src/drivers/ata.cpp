#include <drivers/ata.h>

ATA* ATA::from(u16 port_base, bool master)
{
    ATA* ata = (ATA*)kmalloc(sizeof(ATA));

    ata->data = port_base + 0;
    ata->error = port_base + 1;
    ata->sector_count = port_base + 2;
    ata->lba_low = port_base + 3;
    ata->lba_mid = port_base + 4;
    ata->lba_high = port_base + 5;
    ata->device = port_base + 6;
    ata->command = port_base + 7;
    ata->control = port_base + 0x206;
    ata->byte_per_sector = 512;
    ata->master = master;

    return ata;
}

bool ATA::identify()
{
    outb(this->device, this->master ? 0xa0 : 0xb0);
    outb(this->control, 0x00);

    outb(this->device, 0xa0);
    uint8_t status = inb(this->command);

    if (status == 0xFF)
        return false;

    outb(this->device, this->master ? 0xa0 : 0xb0);
    outb(this->sector_count, 0x00);
    outb(this->lba_low, 0x00);
    outb(this->lba_mid, 0x00);
    outb(this->lba_high, 0x00);
    outb(this->command, 0xec);

    status = inb(this->command);

    if (status == 0x00) // No drive found
        return false;

    while ((status & 0x80) == 0x80 && (status & 0x01) != 0x01)
        status = inb(this->command);

    if (status & 0x01) // Error while identifying ATA
        return false;

    for (int i = 0; i < 256; i++)
        inw(this->data);

    return true;
}

bool ATA::read(u32 lba, u8* buf, u32 count)
{
    if (lba & 0xf0000000)
        return false;

    outb(this->device, (this->master ? 0xe0 : 0xf0) | (lba & 0x0f00000) >> 24);
    outb(this->error, 0);
    outb(this->sector_count, 1);

    outb(this->lba_low, lba & 0x000000ff);
    outb(this->lba_mid, (lba & 0x0000ff00) >> 8);
    outb(this->lba_high, (lba & 0x00ff0000) >> 16);
    outb(this->command, 0x20);

    u8 status = inb(this->command);

    while ((status & 0x80) == 0x80 && (status & 0x01) != 0x01)
        status = inb(this->command);

    if (status & 0x01) // Error while reading
        return false;

    for (u32 i = 0; i < count; i += 2)
    {
        u16 rdata = inw(this->data);

        buf[i] = rdata & 0x00ff;

        if (i + 1 < count)
            buf[i + 1] = (rdata >> 8) & 0x00ff;
    }

    for (u32 i = count + (count % 2); i < 512; i += 2)
        inw(this->data);

    return true;
}

bool ATA::write(u32 lba, u8* buf, u32 count)
{
    if (lba & 0xf0000000)
        return false;

    if (count > 512)
        return false;

    outb(this->device, (this->master ? 0xe0 : 0xf0) | (lba & 0x0f00000) >> 24);
    outb(this->error, 0);
    outb(this->sector_count, 1);

    outb(this->lba_low, lba & 0x000000ff);
    outb(this->lba_mid, (lba & 0x0000ff00) >> 8);
    outb(this->lba_high, (lba & 0x00ff0000) >> 16);
    outb(this->command, 0x30);

    for (u32 i = 0; i < count; i += 2)
    {
        u16 wdata = buf[i];

        if (i + 1 < count)
            wdata |= ((u16)buf[i + 1]) << 8;

        outw(this->data, wdata);
    }

    for (u32 i = count + (count % 2); i < 512; i += 2)
        outw(this->data, 0x0000);

    return true;
}

bool ATA::flush()
{
    outb(this->device, this->master ? 0xe0 : 0xf0);
    outb(this->command, 0xe7);

    uint8_t status = inb(this->command);

    while ((status & 0x80) == 0x80 && (status & 0x01) != 0x01)
        status = inb(this->command);

    if (status & 0x01) // Error while flushing
        return false;

    return true;
}