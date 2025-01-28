#pragma once

#include <limine.h>
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

    enum EscapeState
    {
        NONE,
        EXPECT_CSI,
        TAKE_PARAMS,
    };

    EscapeState state;
    int param_index;
    int params[4];

    void init();
    void clear();
    void scroll();
    void write(const char* buffer, size_t len);
    void ansi_function(char name, int arg);
    void putchar(char c);

    void draw_bitmap(char c);
    void render();
};

extern Framebuffer default_fb;
extern uint32_t* backbuffer;
extern FramebufferTerminal fbterm;