#pragma once

#include <ds/table.h>

struct Superblock;

struct FilesystemType
{
    const char* name;
    Table<Superblock> superblocks;
};

extern Table<FilesystemType> fs_types;