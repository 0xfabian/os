#pragma once

#include <types.h>

struct PSF2
{
    struct Header
    {
        u8 magic[4];
        u32 version;
        u32 header_size;
        u32 flags;
        u32 length;
        u32 char_size;
        u32 height;
        u32 width;
    };

    Header* header;
    u8* glyph_buffer;
};

extern PSF2 zap_light20;
extern PSF2 sf_mono20;
extern PSF2 sf_mono24;
extern PSF2 bios8;
extern PSF2 vga16;