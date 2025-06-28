#include <drivers/special.h>
#include <fs/inode.h>

i64 null_read(File* file, void* buf, usize size, usize offset)
{
    return 0;
}

i64 null_write(File* file, const void* buf, usize size, usize offset)
{
    return size;
}

i64 zero_read(File* file, void* buf, usize size, usize offset)
{
    memset(buf, 0, size);
    return size;
}

i64 zero_write(File* file, const void* buf, usize size, usize offset)
{
    return size;
}

u64 random_seed = 88172645463325252ull;

u8 random_byte()
{
    random_seed = random_seed * 6364136223846793005ull + 1;
    return random_seed >> 32;
}

i64 random_read(File* file, void* buf, usize size, usize offset)
{
    u8* bytes = (u8*)buf;

    for (usize i = 0; i < size; i++)
        bytes[i] = random_byte();

    return size;
}

i64 random_write(File* file, const void* buf, usize size, usize offset)
{
    return size;
}

void give_null_fops(FileOps* fops)
{
    fops->open = nullptr;
    fops->close = nullptr;
    fops->read = null_read;
    fops->write = null_write;
    fops->seek = nullptr;
    fops->iterate = nullptr;
    fops->ioctl = nullptr;
}

void give_zero_fops(FileOps* fops)
{
    fops->open = nullptr;
    fops->close = nullptr;
    fops->read = zero_read;
    fops->write = zero_write;
    fops->seek = nullptr;
    fops->iterate = nullptr;
    fops->ioctl = nullptr;
}

void give_random_fops(FileOps* fops)
{
    fops->open = nullptr;
    fops->close = nullptr;
    fops->read = random_read;
    fops->write = random_write;
    fops->seek = nullptr;
    fops->iterate = nullptr;
    fops->ioctl = nullptr;
}
