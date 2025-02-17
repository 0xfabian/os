#include <fs/fd.h>

int FDTable::get_unused()
{
    for (int fd = 0; fd < FD_TABLE_SIZE; fd++)
        if (files[fd] == nullptr)
            return fd;

    kprintf(WARN "get_unused(): file descriptor table is full\n");
    return -1;
}

File* FDTable::get(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
    {
        kprintf(WARN "get(): bad fd %u\n", fd);
        return nullptr;
    }

    return files[fd];
}

int FDTable::install(unsigned int fd, File* file)
{
    if (fd >= FD_TABLE_SIZE)
    {
        kprintf(WARN "install(): bad fd %u\n", fd);
        return -1;
    }

    if (files[fd])
    {
        kprintf(WARN "install(): fd %u is already in use\n", fd);
        return -2;
    }

    files[fd] = file;

    return 0;
}

File* FDTable::uninstall(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
    {
        kprintf(WARN "uninstall(): bad fd %u\n", fd);
        return nullptr;
    }

    File* file = files[fd];

    files[fd] = nullptr;

    return file;
}