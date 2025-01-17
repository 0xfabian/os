#pragma once

#include <limine.h>
#include <memops.h>
#include <font.h>

struct Framebuffer
{
    uint32_t width;
    uint32_t height;
    uint32_t* addr;

    void init(limine_framebuffer* fb);
};

struct FramebufferTerminal
{
    Framebuffer* fb;
    PSF2_Font* font;

    uint32_t width;
    uint32_t height;
    uint32_t cursor;

    uint32_t fg;
    uint32_t bg;

    // struct Cell
    // {
    //     char chr;
    //     uint8_t color;
    // };

    // Cell* buffer;

    void init();
    void clear();
    void scroll();
    void write(const char* buffer, size_t len);
    void ansi_function(char name, int arg);

    void putchar(char c);
    void draw_bitmap(uint8_t chr);
};

extern Framebuffer default_fb;
extern FramebufferTerminal fbterm;