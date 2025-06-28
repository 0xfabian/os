#pragma once

#include <requests.h>
#include <font.h>
#include <mem/vmm.h>
#include <print.h>
#include <kbd.h>
#include <waitq.h>

#define INPUT_BUFFER_SIZE 256

struct FileOps;

struct Framebuffer
{
    u32 width;
    u32 height;
    u32* addr;

    void init(limine_framebuffer* fb);

    void give_fops(FileOps* fops);
};

struct FramebufferTerminal
{
    Framebuffer* fb;
    u32* frontbuffer;
    u32* frontbuffer_end;
    u32* backbuffer;
    u32* backbuffer_end;
    u32* backbuffer_pos;
    bool needs_render;

    PSF2* font;

    u32 width;
    u32 height;
    u32 cursor;
    u32 write_cursor;
    bool cursor_visible;

    u32 fg;
    u32 bg;
    u32* fg_ptr;
    u32* bg_ptr;

    enum EscapeState
    {
        NONE,
        EXPECT_CSI,
        TAKE_PARAMS,
    };

    EscapeState state;
    int param_index;
    int params[4];

    bool line_buffered;
    bool echo;
    int fg_group;

    char input_buffer[INPUT_BUFFER_SIZE];
    usize input_cursor;
    bool eof_received;

    WaitQueue readers_queue;

    void init();
    void enable_backbuffer();

    void clear();
    void clear_to_eol();
    void scroll();

    void write(const void* buffer, usize len);
    isize read(void* buffer, usize len);

    void ansi_function(char name, int arg);
    void putchar(char ch);
    void receive_char(char ch);
    void clear_input();
    isize get_read_size();
    void tick();

    void draw_bitmap(char ch);
    void draw_cursor(bool on);
    void render();

    void give_fops(FileOps* fops);
};

extern Framebuffer default_fb;
extern FramebufferTerminal fbterm;
