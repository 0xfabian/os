#pragma once

#include <cstdint>
#include <cstddef>

struct PSF2
{
    struct Header
    {
        uint8_t magic[4];
        uint32_t version;
        uint32_t header_size;
        uint32_t flags;
        uint32_t length;
        uint32_t char_size;
        uint32_t height;
        uint32_t width;
    };

    Header* header;
    uint8_t* glyph_buffer;
};

extern uint8_t zap_light20_data[];
extern PSF2 zap_light20;