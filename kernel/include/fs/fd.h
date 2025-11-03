#pragma once

#include <print.h>
#include <fs/file.h>

#define FD_TABLE_SIZE 64

struct FDTable
{
    File* files[FD_TABLE_SIZE] = { nullptr };

    int get_unused();
    int get_unused(usize count, unsigned int* fds);
    result_ptr<File> get(unsigned int fd);

    int install(unsigned int fd, File* file);
    result_ptr<File> uninstall(unsigned int fd);

    void init();
    void close_all();
    void dup(const FDTable* other);
    usize size();
};
