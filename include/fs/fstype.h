#pragma once

#include <cstdint>
#include <cstddef>
#include <string.h>
#include <print.h>

#define FS_REQUIRES_DEV 1

struct Superblock;
struct Device;

struct Filesystem
{
    const char* name;
    uint32_t flags;
    size_t num_sb;

    Superblock* (*create_sb)(Filesystem* fs, Device* dev);
    void (*destroy_sb)(Superblock* sb);

    static Filesystem* find(const char* name);

    bool requires_device();
    void register_self();
    void unregister();
};

#define FS_TABLE_SIZE 16

struct FilesystemTable
{
    Filesystem* filesystems[FS_TABLE_SIZE];

    void add(Filesystem* fs);
    void remove(Filesystem* fs);
    Filesystem* find(const char* name);

    void debug();
};

extern FilesystemTable fs_table;