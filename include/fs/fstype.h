#pragma once

#include <types.h>
#include <string.h>
#include <print.h>
#include <error.h>
#include <drivers/bdev.h>

#define FS_REQUIRES_DEV 1

struct Superblock;

struct Filesystem
{
    const char* name;
    u32 flags;
    result_ptr<Superblock>(*create_sb)(Filesystem* fs, BlockDevice* dev);
    void (*destroy_sb)(Superblock* sb);

    bool registered;
    usize num_sb;
    Filesystem* next;

    static Filesystem* find(const char* name);

    int register_self();
    int unregister();

    bool requires_device();
};

extern Filesystem* fs_list;

void debug_filesystems();