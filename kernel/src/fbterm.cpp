#include <fbterm.h>
#include <task.h>
#include <fs/inode.h>

// vga
// #define BLACK               0x000000
// #define RED                 0xaa0000
// #define GREEN               0x00aa00
// #define YELLOW              0xaa5500
// #define BLUE                0x0000aa
// #define MAGENTA             0xaa00aa
// #define CYAN                0x00aaaa
// #define WHITE               0xaaaaaa
// #define BRIGHT_BLACK        0x555555
// #define BRIGHT_RED          0xff5555
// #define BRIGHT_GREEN        0x55ff55
// #define BRIGHT_YELLOW       0xffff55
// #define BRIGHT_BLUE         0x5555ff
// #define BRIGHT_MAGENTA      0xff55ff
// #define BRIGHT_CYAN         0x55ffff
// #define BRIGHT_WHITE        0xffffff
// #define BACKGROUND          0x000000
// #define FOREGROUND          0xaaaaaa

// gruvbox
#define BLACK               0x282828
#define RED                 0xcc241d
#define GREEN               0x98971a
#define YELLOW              0xd79921
#define BLUE                0x458588
#define MAGENTA             0xb16286
#define CYAN                0x689d6a
#define WHITE               0x928374
#define BRIGHT_BLACK        0x3c3836
#define BRIGHT_RED          0xfb4934
#define BRIGHT_GREEN        0xb8bb26
#define BRIGHT_YELLOW       0xfabd2f
#define BRIGHT_BLUE         0x83a598
#define BRIGHT_MAGENTA      0xd3869b
#define BRIGHT_CYAN         0x8ec07c
#define BRIGHT_WHITE        0xebdbb2
#define BACKGROUND          0x1d2021
#define FOREGROUND          0xfbf1c7

// chocolate
// #define BLACK               0x2a1f15
// #define RED                 0xbe2d26
// #define GREEN               0x97b349
// #define YELLOW              0xe99d2a
// #define BLUE                0x5a86ad
// #define MAGENTA             0xac80a6
// #define CYAN                0x6ba18a
// #define WHITE               0x9b6c4a
// #define BRIGHT_BLACK        0x573d26
// #define BRIGHT_RED          0xe84627
// #define BRIGHT_GREEN        0xd0d150
// #define BRIGHT_YELLOW       0xffc745
// #define BRIGHT_BLUE         0xb8d3ed
// #define BRIGHT_MAGENTA      0xd19ecb
// #define BRIGHT_CYAN         0x95d8ba
// #define BRIGHT_WHITE        0xfff9d5
// #define BACKGROUND          BLACK
// #define FOREGROUND          BRIGHT_WHITE

u32 colors[] =
{
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    BRIGHT_BLACK,
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_WHITE
};

Framebuffer default_fb;
FramebufferTerminal fbterm;
int cursor_ticks = 0;

void Framebuffer::init(limine_framebuffer* fb)
{
    width = fb->width;
    height = fb->height;
    addr = (u32*)fb->address;
}

isize fb_read(File* file, void* buf, usize size, usize offset)
{
    if (size == 0 || offset >= default_fb.width * default_fb.height * sizeof(u32))
        return 0;

    usize remaining = default_fb.width * default_fb.height * sizeof(u32) - offset;

    if (size > remaining)
        size = remaining;

    memcpy(buf, (u8*)default_fb.addr + offset, size);

    return size;
}

isize fb_write(File* file, const void* buf, usize size, usize offset)
{
    if (size == 0)
        return 0;

    offset %= default_fb.width * default_fb.height * sizeof(u32);

    usize remaining = default_fb.width * default_fb.height * sizeof(u32) - offset;

    if (size > remaining)
        size = remaining;

    memcpy((u8*)default_fb.addr + offset, buf, size);

    return size;
}

usize fb_seek(File* file, usize offset, int whence)
{
    usize size = default_fb.width * default_fb.height;

    if (offset > size)
        offset = size;

    file->offset = offset * sizeof(u32);

    return file->offset;
}

#define FB_GET_WIDTH    1
#define FB_GET_HEIGHT   2

int fb_ioctl(File* file, int cmd, void* arg)
{
    switch (cmd)
    {
    case FB_GET_WIDTH:  *(u32*)arg = default_fb.width;      break;
    case FB_GET_HEIGHT: *(u32*)arg = default_fb.height;     break;
    default:                                                return -1;
    }

    return 0;
}

void Framebuffer::give_fops(FileOps* fops)
{
    fops->open = nullptr;
    fops->close = nullptr;
    fops->read = fb_read;
    fops->write = fb_write;
    fops->seek = fb_seek;
    fops->iterate = nullptr;
    fops->ioctl = fb_ioctl;
}

void FramebufferTerminal::init()
{
    limine_framebuffer_response* framebuffer_response = framebuffer_request.response;

    if (!framebuffer_response || framebuffer_response->framebuffer_count == 0)
        idle();

    default_fb.init(framebuffer_response->framebuffers[0]);

    fb = &default_fb;
    font = &sf_mono20;

    width = fb->width / font->header->width;
    height = fb->height / font->header->height;
    cursor = 0;
    write_cursor = 0;
    cursor_visible = true;

    frontbuffer = fb->addr;
    frontbuffer_end = fb->addr + fb->width * fb->height;
    backbuffer = nullptr;
    backbuffer_end = nullptr;
    backbuffer_pos = nullptr;

    needs_render = false;

    fg = FOREGROUND;
    bg = BACKGROUND;
    fg_ptr = &fg;
    bg_ptr = &bg;

    state = NONE;
    param_index = 0;
    params[0] = 0;
    params[1] = 0;
    params[2] = 0;
    params[3] = 0;

    line_buffered = true;
    echo = true;
    fg_group = 0;

    input_cursor = 0;

    readers_queue.init();

    clear();

    kprintf(INFO "Framebuffer terminal initialized (%ux%u)\n", width, height);
}

void FramebufferTerminal::enable_backbuffer()
{
    kprintf(INFO "Enabling backbuffer...\n");

    usize count = PAGE_COUNT(default_fb.width * default_fb.height * sizeof(u32));
    backbuffer = (u32*)vmm.alloc_pages(count, PE_WRITE | 1 << 7 | 1 << 3);

    if (!backbuffer)
    {
        kprintf(WARN "Failed to allocate backbuffer\n");
        return;
    }

    backbuffer_end = backbuffer + fb->width * fb->height;
    backbuffer_pos = backbuffer;

    // this is very slow because we are reading from the framebuffer
    // but i don't now any better way to sync the two buffers

    u64* from = (u64*)frontbuffer;
    u64* to = (u64*)backbuffer;

    while (from < (u64*)frontbuffer_end)
        *to++ = *from++;
}

void FramebufferTerminal::clear()
{
    u64 value = ((u64)*bg_ptr << 32) | *bg_ptr;

    for (u64* ptr = (u64*)frontbuffer; ptr < (u64*)frontbuffer_end; ptr++)
        *ptr = value;

    if (backbuffer)
    {
        for (u64* ptr = (u64*)backbuffer; ptr < (u64*)backbuffer_end; ptr++)
            *ptr = value;

        backbuffer_pos = backbuffer;
    }

    cursor = 0;
    write_cursor = 0;
}

void FramebufferTerminal::clear_to_eol()
{
    u64 value = ((u64)*bg_ptr << 32) | *bg_ptr;
    u32 line = (cursor / width) * font->header->height;
    u32 start = (cursor % width) * font->header->width;

    u32* ptr = frontbuffer + line * fb->width;

    for (u32 y = 0; y < font->header->height; y++)
    {
        for (u32 x = start; x < fb->width; x++)
            ptr[x] = value;

        ptr += fb->width;
    }

    if (backbuffer)
    {
        ptr = backbuffer_pos + line * fb->width;

        if (ptr >= backbuffer_end)
            ptr -= backbuffer_end - backbuffer;

        for (u32 y = 0; y < font->header->height; y++)
        {
            for (u32 x = start; x < fb->width; x++)
                ptr[x] = value;

            ptr += fb->width;
        }
    }
}

void FramebufferTerminal::scroll()
{
    u64 value = ((u64)*bg_ptr << 32) | *bg_ptr;
    u64 scroll_offset = fb->width * font->header->height;

    if (!backbuffer)
    {
        // we need to divide by 2 because we are using 64-bit writes
        u64 scroll_offset_2 = scroll_offset >> 1;

        for (u64* ptr = (u64*)frontbuffer; ptr < (u64*)(frontbuffer_end - scroll_offset); ptr++)
            *ptr = *(ptr + scroll_offset_2);

        for (u64* ptr = (u64*)(frontbuffer_end - scroll_offset); ptr < (u64*)frontbuffer_end; ptr++)
            *ptr = value;
    }
    else
    {
        u64* ptr;

        for (ptr = (u64*)backbuffer_pos; ptr < (u64*)(backbuffer_pos + scroll_offset); ptr++)
            *ptr = value;

        backbuffer_pos = (u32*)ptr;

        if (backbuffer_pos >= backbuffer_end)
            backbuffer_pos = backbuffer;

        needs_render = true;
    }

    cursor -= width;
    write_cursor -= width;
}

void FramebufferTerminal::write(const void* buffer, usize len)
{
    const u8* ptr = (const u8*)buffer;
    usize i = 0;

    draw_cursor(false);

    while (i < len)
    {
        switch (state)
        {
        case NONE:

            if (*ptr == '\e')
                state = EXPECT_CSI;
            else
                putchar(*ptr);

            break;

        case EXPECT_CSI:

            if (*ptr == '[')
            {
                state = TAKE_PARAMS;

                param_index = 0;
                params[0] = 0;
                params[1] = 0;
                params[2] = 0;
                params[3] = 0;
            }
            else
            {
                state = NONE;

                putchar('\e');
                putchar(*ptr);
            }

            break;

        case TAKE_PARAMS:

            if (*ptr >= '0' && *ptr <= '9')
            {
                params[param_index] = params[param_index] * 10 + *ptr - '0';
            }
            else if (*ptr == ';')
            {
                if (param_index < 4)
                    param_index++;
            }
            else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z'))
            {
                for (int j = 0; j <= param_index; j++)
                    ansi_function(*ptr, params[j], j);

                state = NONE;
            }

            break;
        }

        ptr++;
        i++;
    }

    draw_cursor(true);
    cursor_ticks = 0;
}

// all of this ANSI stuff is a mess
isize FramebufferTerminal::read(void* buffer, usize len)
{
    while (true)
    {
        isize read = get_read_size();

        if (read >= 0)
        {
            if ((usize)read > len)
                read = len;

            memcpy(buffer, input_buffer, read);
            input_cursor -= read;
            memmove(input_buffer, input_buffer + read, input_cursor);
            eof_received = false;

            return read;
        }

        wait_on(&readers_queue);
    }
}

inline int min(int a, int b)
{
    return a < b ? a : b;
}

inline int max(int a, int b)
{
    return a > b ? a : b;
}

void FramebufferTerminal::ansi_function(char name, int arg, int param_index)
{
    switch (name)
    {
    case 'm':

        if (arg >= 30 && arg <= 37)
            fg = colors[arg - 30];
        else if (arg == 39)
            fg = FOREGROUND;
        else if (arg >= 40 && arg <= 47)
            bg = colors[arg - 40];
        else if (arg == 49)
            bg = BACKGROUND;
        else if (arg >= 90 && arg <= 97)
            fg = colors[arg - 90 + 8];
        else if (arg >= 100 && arg <= 107)
            bg = colors[arg - 100 + 8];
        else if (arg == 0)
        {
            fg = FOREGROUND;
            bg = BACKGROUND;

            fg_ptr = &fg;
            bg_ptr = &bg;
        }
        else if (arg == 7)
        {
            fg_ptr = &bg;
            bg_ptr = &fg;
        }
        else if (arg == 27)
        {
            fg_ptr = &fg;
            bg_ptr = &bg;
        }

        break;

    case 'H':

        if (param_index == 0)
        {
            int line = min(max(arg - 1, 0), height - 1);
            cursor = line * width;
        }
        else if (param_index == 1)
        {
            int col = min(max(arg - 1, 0), width - 1);
            cursor += col;
        }

        break;

    case 'K':
        clear_to_eol();
        break;
    case 'h':
        if (arg == 25)
            cursor_visible = true;
        break;
    case 'l':
        if (arg == 25)
            cursor_visible = false;
        break;
    }
}

void FramebufferTerminal::putchar(char ch)
{
    if ((u8)ch >= ' ')
    {
        draw_bitmap(ch);
        cursor++;
    }
    else
    {
        if (ch == '\n')
        {
            cursor += width - cursor % width;
        }
        else if (ch == '\r')
        {
            cursor -= cursor % width;
        }
        else if (ch == '\b')
        {
            if (cursor > write_cursor)
            {
                cursor--;
                draw_bitmap(' ');
            }
        }
        else if (ch == '\a')
        {
            if (fbterm.cursor % fbterm.width)
                cursor += width - cursor % width;
        }
        // we should handle tabs and also maybe
        // print something for non-printable characters
    }

    if (cursor >= width * height)
    {
        if (line_buffered)
            scroll();
        else
            cursor = width * height - 1;
    }
}

void FramebufferTerminal::receive_char(char ch)
{
    if (line_buffered)
    {
        if (ch == '\b')
        {
            if (input_cursor > 0)
                input_cursor--;
            else
                return;
        }
        else if (input_cursor < INPUT_BUFFER_SIZE)
        {
            input_buffer[input_cursor++] = ch;

            if (ch == '\n')
                readers_queue.wake_all();
        }
    }
    else if (input_cursor < INPUT_BUFFER_SIZE)
        input_buffer[input_cursor++] = ch;

    if (echo)
        write(&ch, 1);
}

void FramebufferTerminal::clear_input()
{
    while (input_cursor)
    {
        write("\b", 1);
        input_cursor--;
    }
}

isize FramebufferTerminal::get_read_size()
{
    if (!line_buffered || eof_received)
        return input_cursor;

    for (usize i = 0; i < input_cursor; i++)
        if (input_buffer[i] == '\n')
            return i + 1;

    return -1;
}

void FramebufferTerminal::tick()
{
    if (needs_render)
    {
        render();
        needs_render = false;
    }
    else if (cursor_ticks % 20 == 0)
        draw_cursor(cursor_ticks % 40 == 0);

    cursor_ticks++;
}

inline u32 alpha_blend(u32 c1, u32 c2, u8 alpha)
{
    // hacky way to do alpha blending
    // this is not really accurate but it's fast

    u64 rgb1 = (u64)(c1 & 0x00ff00) << 24 | (c1 & 0xff00ff);
    u64 rgb2 = (u64)(c2 & 0x00ff00) << 24 | (c2 & 0xff00ff);

    u64 rgb = ((rgb1 - rgb2) * alpha >> 8) + rgb2;

    return (rgb & 0xff00ff) | ((rgb >> 24) & 0x00ff00);
}

void FramebufferTerminal::draw_bitmap(char ch)
{
    u32 x = (cursor % width) * font->header->width;
    u32 y = (cursor / width) * font->header->height;

    u8* font_ptr = font->glyph_buffer + (u8)ch * font->header->char_size;
    u32* front_ptr = frontbuffer + x + y * fb->width;
    u32* draw_ptr;

    if (backbuffer)
    {
        draw_ptr = backbuffer_pos + x + y * fb->width;

        if (draw_ptr >= backbuffer_end)
            draw_ptr -= backbuffer_end - backbuffer;
    }
    else
        draw_ptr = front_ptr;

    // // old bitmap drawing
    // for (y = 0; y < font->header->height; y++)
    // {
    //     for (x = 0; x < font->header->width; x++)
    //     {
    //         if (x == 8)
    //             font_ptr++;

    //         draw_ptr[x] = (*font_ptr & (0b10000000 >> (x & 7))) ? *fg_ptr : *bg_ptr;
    //     }

    //     font_ptr++;
    //     draw_ptr += fb->width;
    // }

    // new alpha blended drawing
    for (y = 0; y < font->header->height; y++)
    {
        for (x = 0; x < font->header->width; x++)
            draw_ptr[x] = alpha_blend(*fg_ptr, *bg_ptr, font_ptr[x]);

        font_ptr += font->header->width;
        draw_ptr += fb->width;
    }

    if (!backbuffer || needs_render)
        return;

    // at this point we have a backbuffer and a render call is not guaranteed
    // so we manually copy the drawn character to the frontbuffer

    draw_ptr -= fb->width * font->header->height;

    for (y = 0; y < font->header->height; y++)
    {
        for (x = 0; x < font->header->width; x++)
            front_ptr[x] = draw_ptr[x];

        front_ptr += fb->width;
        draw_ptr += fb->width;
    }
}

void FramebufferTerminal::draw_cursor(bool on)
{
    if (!cursor_visible)
        return;

    u32 x = (cursor % width) * font->header->width;
    u32 y = (cursor / width) * font->header->height;

    u32* ptr = frontbuffer + x + y * fb->width;

    for (y = 0; y < font->header->height; y++)
    {
        for (x = 0; x < font->header->width; x++)
            ptr[x] = on ? *fg_ptr : *bg_ptr;

        ptr += fb->width;
    }
}

void FramebufferTerminal::render()
{
    u64* to = (u64*)frontbuffer;

    for (u64* ptr = (u64*)backbuffer_pos; ptr < (u64*)backbuffer_end; ptr++)
        *to++ = *ptr;

    for (u64* ptr = (u64*)backbuffer; ptr < (u64*)backbuffer_pos; ptr++)
        *to++ = *ptr;
}

isize fbterm_read(File* file, void* buf, usize size, usize offset)
{
    return fbterm.read(buf, size);
}

isize fbterm_write(File* file, const void* buf, usize size, usize offset)
{
    fbterm.write(buf, size);
    fbterm.write_cursor = fbterm.cursor;

    return size;
}

#define FBTERM_CLEAR        1
#define FBTERM_ECHO_ON      2
#define FBTERM_ECHO_OFF     3
#define FBTERM_LB_ON        4
#define FBTERM_LB_OFF       5
#define FBTERM_GET_WIDTH    6
#define FBTERM_GET_HEIGHT   7
#define FBTERM_SET_FG_GROUP 8

int fbterm_ioctl(File* file, int cmd, void* arg)
{
    switch (cmd)
    {
    case FBTERM_CLEAR:          fbterm.clear();                 break;
    case FBTERM_ECHO_ON:        fbterm.echo = true;             break;
    case FBTERM_ECHO_OFF:       fbterm.echo = false;            break;
    case FBTERM_LB_ON:          fbterm.line_buffered = true;    break;
    case FBTERM_LB_OFF:         fbterm.line_buffered = false;   break;
    case FBTERM_GET_WIDTH:      *(int*)arg = fbterm.width;      break;
    case FBTERM_GET_HEIGHT:     *(int*)arg = fbterm.height;     break;
    case FBTERM_SET_FG_GROUP:   fbterm.fg_group = *(int*)arg;   break;
    default:                                                    return -1;
    }

    return 0;
}

void FramebufferTerminal::give_fops(FileOps* fops)
{
    fops->open = nullptr;
    fops->close = nullptr;
    fops->read = fbterm_read;
    fops->write = fbterm_write;
    fops->seek = nullptr;
    fops->iterate = nullptr;
    fops->ioctl = fbterm_ioctl;
}
