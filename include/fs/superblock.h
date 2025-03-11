#pragma once

#include <fs/fstype.h>

struct Inode;

struct SuperblockOps
{
    int (*sync) (struct Superblock* sb);
};

struct Superblock
{
    Filesystem* fs;
    BlockDevice* dev;
    Inode* root;
    void* data;

    SuperblockOps ops;

    void destroy();
};