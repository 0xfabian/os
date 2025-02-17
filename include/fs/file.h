#pragma once

#include <print.h>
#include <fs/inode.h>

struct File;

struct FileOps
{
    int (*open) (File* file);
    int (*close) (File* file);
    int (*read) (File* file, char* buf, size_t size, size_t offset);
    int (*write) (File* file, const char* buf, size_t size, size_t offset);
    int (*seek) (File* file, size_t offset, int whence);
};

struct File
{
    size_t offset;
    Inode* inode;
    uint32_t flags;
    int refs;

    FileOps ops;

    static File* get(const char* path, uint32_t flags);

    int open();
    int close();
    int read(char* buf, size_t size);
    int write(const char* buf, size_t size);
    int seek(size_t offset, int whence);
};

#define FILE_TABLE_SIZE 256

struct FileTable
{
    File files[FILE_TABLE_SIZE];

    File* alloc();

    void debug();
};

extern FileTable file_table;