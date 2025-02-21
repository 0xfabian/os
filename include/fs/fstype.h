#pragma once

#include <cstdint>
#include <cstddef>
#include <string.h>
#include <print.h>
#include <error.h>

#define FS_REQUIRES_DEV 1

struct Superblock;
struct Device;

struct Filesystem
{
    const char* name;
    uint32_t flags;
    result_ptr<Superblock>(*create_sb)(Filesystem* fs, Device* dev);
    void (*destroy_sb)(Superblock* sb);

    bool registered;
    size_t num_sb;
    Filesystem* next;

    static Filesystem* find(const char* name);
    static void debug();

    int register_self();
    int unregister();

    bool requires_device();
};

extern Filesystem* fs_list;