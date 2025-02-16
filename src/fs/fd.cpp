#include <fs/fd.h>

int FDTable::get_unused()
{
    for (int fd = 0; fd < FD_TABLE_SIZE; fd++)
        if (files[fd] == nullptr)
            return fd;

    return -1;
}

File* FDTable::get(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
        return nullptr;

    return files[fd];
}

int FDTable::install(unsigned int fd, File* file)
{
    if (fd >= FD_TABLE_SIZE)
        return -1;

    if (files[fd])
        return -2;

    files[fd] = file;

    return 0;
}

File* FDTable::uninstall(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
        return nullptr;

    File* file = files[fd];

    files[fd] = nullptr;

    return file;
}