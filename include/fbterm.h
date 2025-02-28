#pragma once

#include <requests.h>
#include <font.h>
#include <mem/pmm.h>
#include <print.h>

struct Framebuffer
{
    u32 width;
    u32 height;
    u32* addr;

    void init(limine_framebuffer* fb);
};

struct FramebufferTerminal
{
    Framebuffer* fb;
    PSF2* font;

    u32 width;
    u32 height;
    u32 cursor;

    u32 fg;
    u32 bg;

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
    void enable_backbuffer();

    void clear();
    void scroll();
    void write(const char* buffer, usize len);
    void ansi_function(char name, int arg);
    void putchar(char c);

    void draw_bitmap(char c);
    void render();
};

extern Framebuffer default_fb;
extern u32* backbuffer;
extern FramebufferTerminal fbterm;