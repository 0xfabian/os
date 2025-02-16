#pragma once

#include <fs/file.h>

#define FD_TABLE_SIZE 64

struct FDTable
{
    File* files[FD_TABLE_SIZE] = { nullptr };

    int get_unused();
    File* get(unsigned int fd);

    int install(unsigned int fd, File* file);
    File* uninstall(unsigned int fd);
};