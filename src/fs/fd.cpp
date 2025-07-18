#include <fs/fd.h>

int FDTable::get_unused()
{
    for (int fd = 0; fd < FD_TABLE_SIZE; fd++)
        if (files[fd] == nullptr)
            return fd;

    return -ERR_FD_TABLE_FULL;
}

int FDTable::get_unused(usize count, unsigned int* fds)
{
    if (count == 0)
        return 0;

    usize filled = 0;

    for (int fd = 0; fd < FD_TABLE_SIZE; fd++)
    {
        if (files[fd] == nullptr)
        {
            fds[filled++] = fd;

            if (filled == count)
                return 0;
        }
    }

    return -ERR_FD_TABLE_FULL;
}

result_ptr<File> FDTable::get(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE || !files[fd])
        return -ERR_BAD_FD;

    return files[fd];
}

int FDTable::install(unsigned int fd, File* file)
{
    if (fd >= FD_TABLE_SIZE)
        return -ERR_BAD_FD;

    if (files[fd])
        files[fd]->close();

    files[fd] = file;

    return 0;
}

result_ptr<File> FDTable::uninstall(unsigned int fd)
{
    if (fd >= FD_TABLE_SIZE)
        return -ERR_BAD_FD;

    File* file = files[fd];

    if (!file)
        return -ERR_BAD_FD;

    files[fd] = nullptr;

    return file;
}

void FDTable::init()
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
        files[i] = nullptr;
}

void FDTable::close_all()
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (files[i])
        {
            files[i]->close();
            files[i] = nullptr;
        }
    }
}

void FDTable::dup(const FDTable* other)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
        if (other->files[i])
            files[i] = other->files[i]->dup();
}

usize FDTable::size()
{
    usize count = 0;

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        if (files[i])
            count++;

    return count;
}
