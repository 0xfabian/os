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
    usize submounts;

    static Mount* find(Inode* inode);
    static Mount* find(Superblock* sb);

    static int fill_mount(Mount* mnt, Device* dev, Filesystem* fs);
    static result_ptr<Mount> mount(const char* target, Device* dev, Filesystem* fs);
    static result_ptr<Mount> mount_root(Device* dev, Filesystem* fs);
    int unmount();
};

#define MOUNT_TABLE_SIZE 64

extern Mount mounts[MOUNT_TABLE_SIZE];
extern Mount* root_mount;

void debug_mounts();