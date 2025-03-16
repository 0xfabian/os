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
struct FileOps;

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
    void putchar(char ch);
    void receive_char(char ch);
    void clear_input();
    void tick();
    void add_request(char* buffer, usize len);
    void handle_requests();

    void draw_bitmap(char ch);
    void draw_cursor(u32 color);
    void render();

    void give_fops(FileOps* fops);
};

extern Framebuffer default_fb;
extern FramebufferTerminal fbterm;