#pragma once

#include <fs/fstype.h>
#include <fs/inode.h>
#include <fs/superblock.h>

struct Mountpoint
{
    char* path;
    Inode* inode;
};

struct Mount
{
    Mountpoint mp;
    Superblock* sb;
    Mount* parent;
    size_t submounts;

    static Mount* find(Inode* inode);
    static Mount* find(Superblock* sb);

    static int fill_mount(Mount* mnt, Device* dev, Filesystem* fs);
    static int mount(const char* target, Device* dev, Filesystem* fs);
    static int mount_root(Device* dev, Filesystem* fs);
    int unmount();

    Inode* get_root();
};

#define MOUNT_TABLE_SIZE 64

struct MountTable
{
    Mount mounts[MOUNT_TABLE_SIZE];

    Mount* alloc();
    Mount* find(Inode* inode);
    Mount* find(Superblock* sb);

    void debug();
};

extern MountTable mount_table;
extern Mount* root_mount;