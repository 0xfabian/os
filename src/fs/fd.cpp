#include <fs/fd.h>

int FDTable::get_unused()
{
    for (int fd = 0; fd < FD_TABLE_SIZE; fd++)
        if (files[fd] == nullptr)
            return fd;

    return -ERR_FD_TABLE_FULL;
}

result_ptr<File> FDTable::get(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
        return -ERR_BAD_FD;

    return files[fd];
}

int FDTable::install(unsigned int fd, File* file)
{
    if (fd >= FD_TABLE_SIZE)
        return -ERR_BAD_FD;

    if (files[fd])
        kprintf(WARN "install(): fd %u is already in use\n", fd);

    files[fd] = file;

    return 0;
}

result_ptr<File> FDTable::uninstall(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
        return -ERR_BAD_FD;

    File* file = files[fd];

    files[fd] = nullptr;

    return file;
}