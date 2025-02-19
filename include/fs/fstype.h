#pragma once

#include <cstdint>
#include <cstddef>
#include <utils.h>
#include <print.h>
// #include <ds/table.h>

struct Superblock;

struct FilesystemType
{
    const char* name;
    size_t active_mounts;
    Superblock* (*mount)(const char* dev);

    FilesystemType* next;
};

void register_fs(FilesystemType* fs);
int unregister_fs(FilesystemType* fs);
FilesystemType* find_fs(const char* name);