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
    Device* dev;
    Inode* root;
    void* data;

    SuperblockOps ops;

    void destroy();
};