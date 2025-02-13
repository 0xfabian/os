#pragma once

#include <ds/table.h>

struct Superblock;

struct Filesystem
{
    const char* name;
    Table<Superblock> superblocks;
};

extern Table<Filesystem*> fs_table;

// void register_filesystem(Filesystem* fs)
// {
//     Filesystem** fs_ptr = fs_table.alloc();
//     *fs_ptr = fs;
// }

// Filesystem* find_filesystem(const char* name)
// {
//     for (auto fs : fs_table)
//         if (fs->name == name)
//             return fs;

//     return nullptr;
// }