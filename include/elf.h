#pragma once

#include <types.h>

#define PT_LOAD     1
#define PT_TLS      7

struct ELF
{
    struct Header
    {
        struct
        {
            u32 magic;
            u8 format;
            u8 data;
            u8 version1;
            u8 osabi;
            u64 pad;
        };

        u16 type;
        u16 machine;
        u32 version2;
        u64 entry;
        u64 phoff;
        u64 shoff;
        u32 flags;
        u16 ehsize;
        u16 phentsize;
        u16 phnum;
        u16 shentsize;
        u16 shnum;
        u16 shstrndx;

        bool supported()
        {
            // 0x7f 'E' 'L' 'F'
            if (magic != 0x464c457f)
                return false;

            // 64-bit
            if (format != 2)
                return false;

            // little-endian
            if (data != 1)
                return false;

            // current version
            if (version1 != 1)
                return false;

            // UNIX System V or Linux
            if (osabi != 0 && osabi != 3)
                return false;

            // executable
            if (type != 2)
                return false;

            // x86_64
            if (machine != 0x3e)
                return false;

            return true;
        }
    }
    __attribute__((packed));

    struct ProgramHeader
    {
        u32 type;
        u32 flags;
        u64 offset;
        u64 vaddr;
        u64 paddr;
        u64 filesz;
        u64 memsz;
        u64 align;
    }
    __attribute__((packed));
};