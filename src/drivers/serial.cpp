#include <drivers/serial.h>
#include <fs/inode.h>
#include <task.h>

#define SERIAL_COM1 0x3F8

int serial_can_receive()
{
    return inb(SERIAL_COM1 + 5) & 0x01;
}

u8 serial_read_byte(i64 timeout)
{
    while (!serial_can_receive() && timeout-- > 0);

    if (timeout <= 0)
        return -1;

    return inb(SERIAL_COM1);
}

int serial_can_transmit()
{
    return inb(SERIAL_COM1 + 5) & 0x20;
}

void serial_write_byte(u8 byte)
{
    while (!serial_can_transmit());

    outb(SERIAL_COM1, byte);
}

int serial_open(File* file)
{
    outb(SERIAL_COM1 + 1, 0x00);    // Disable interrupts
    outb(SERIAL_COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_COM1 + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_COM1 + 2, 0xC7);    // Enable FIFO, clear them, 14-byte threshold
    outb(SERIAL_COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    return 0;
}

isize serial_read(File* file, void* buf, usize size, usize offset)
{
    u8* bytes = (u8*)buf;
    isize bytes_read = 0;

    for (usize i = 0; i < size; i++)
    {
        u8 read = serial_read_byte(100000);

        if (read == 0xff)
            break;

        if (read == '\r')
            read = '\n';

        kprintf("Read byte: %hhx\n", read);
        bytes[bytes_read++] = read;

        if (read == '\n')
            break;
    }

    return bytes_read;
}

isize serial_write(File* file, const void* buf, usize size, usize offset)
{
    const u8* bytes = (const u8*)buf;

    for (usize i = 0; i < size; i++)
        serial_write_byte(bytes[i]);

    return size;
}

void give_serial_fops(FileOps* fops)
{
    fops->open = serial_open;
    fops->close = nullptr;
    fops->read = serial_read;
    fops->write = serial_write;
    fops->seek = nullptr;
    fops->iterate = nullptr;
    fops->ioctl = nullptr;
}