#pragma once

#include <print.h>
#include <string.h>
#include <fs/inode.h>
#include <pipe.h>
#include <drivers/special.h>
#include <drivers/serial.h>

#define O_RDONLY        0
#define O_WRONLY        1
#define O_RDWR          2
#define O_CREAT         0x40
#define O_EXCL          0x80
#define O_TRUNC         0x200
#define O_APPEND        0x400
#define O_DIRECTORY     0x10000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct File
{
    usize offset;
    Inode* inode;
    Pipe* pipe;
    u32 flags;
    int refs;

    FileOps ops;

    static result_ptr<File> open(const char* path, u32 flags, u32 mode = 0);
    int close();

    isize read(void* buf, usize size);
    isize write(const void* buf, usize size);
    isize pread(void* buf, usize size, usize offset);
    isize pwrite(const void* buf, usize size, usize offset);
    usize seek(usize offset, int whence);
    int iterate(void* buf, usize size);
    int ioctl(int cmd, void* arg);

    File* dup();
};

#define FILE_TABLE_SIZE 256

struct FileTable
{
    File files[FILE_TABLE_SIZE];

    result_ptr<File> alloc();

    void debug();
};

extern FileTable file_table;

usize generic_seek(File* file, usize offset, int whence);