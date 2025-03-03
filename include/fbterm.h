#pragma once

#include <requests.h>
#include <font.h>
#include <mem/vmm.h>
#include <print.h>
#include <kbd.h>

#define INPUT_BUFFER_SIZE 256

struct Framebuffer
{
    u32 width;
    u32 height;
    u32* addr;

    void init(limine_framebuffer* fb);
};

struct Task;

struct ReadRequest
{
    Task* task;

    char* buffer;
    usize len;
    usize read;

    ReadRequest* next;
};

struct FramebufferTerminal
{
    Framebuffer* fb;
    u32* backbuffer;
    PSF2* font;

    u32 width;
    u32 height;
    u32 cursor;
    u32 write_cursor;

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

    bool line_buffered;
    bool echo;

    char input_buffer[INPUT_BUFFER_SIZE];
    usize input_cursor;

    ReadRequest* read_queue;

    void init();
    void enable_backbuffer();

    void clear();
    void scroll();

    void write(const char* buffer, usize len);
    isize read(char* buffer, usize len);

    void ansi_function(char name, int arg);
    void putchar(char c);
    void handle_key(int key);
    void blink_cursor();
    void add_request(char* buffer, usize len);
    void handle_requests();

    void draw_bitmap(char c);
    void draw_cursor(u32 color);
    void render();
};

extern Framebuffer default_fb;
extern FramebufferTerminal fbterm;