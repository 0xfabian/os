#include <fbterm.h>
#include <task.h>
#include <fs/inode.h>

// #define FOREGROUND          0xf0ffff
// #define BACKGROUND          0x192840
// #define BLACK               0x1a1c1f
// #define RED                 0xc50f1f
// #define GREEN               0x34a116
// #define YELLOW              0xf2a623
// #define BLUE                0x2b5cb3
// #define MAGENTA             0x8a155e
// #define CYAN                0x3dbab5
// #define WHITE               0xcccccc
// #define BRIGHT_BLACK        0x8d95a3
// #define BRIGHT_RED          0xff4a4a
// #define BRIGHT_GREEN        0x89ff64
// #define BRIGHT_YELLOW       0xffea3a
// #define BRIGHT_BLUE         0x6fa3ff
// #define BRIGHT_MAGENTA      0xb53083
// #define BRIGHT_CYAN         0x79f9de
// #define BRIGHT_WHITE        0xf2f2f2

// gruvbox color theme
// #define FOREGROUND          0xfbf1c7
// #define BACKGROUND          0x282828
// #define BLACK               0x282828
// #define RED                 0xcc241d
// #define GREEN               0x98971a
// #define YELLOW              0xd79921
// #define BLUE                0x458588
// #define MAGENTA             0xb16286
// #define CYAN                0x689d6a
// #define WHITE               0xa89984
// #define BRIGHT_BLACK        0x928374
// #define BRIGHT_RED          0xfb4934
// #define BRIGHT_GREEN        0xb8bb26
// #define BRIGHT_YELLOW       0xfabd2f
// #define BRIGHT_BLUE         0x83a598
// #define BRIGHT_MAGENTA      0xd3869b
// #define BRIGHT_CYAN         0x8ec07c
// #define BRIGHT_WHITE        0xebdbb2

#define FOREGROUND          0xecf2f8
#define BACKGROUND          0x0d1117
#define BLACK               0x21262d
#define RED                 0xc73834
#define GREEN               0x49a648
#define YELLOW              0xb05b28
#define BLUE                0x3d84b0
#define MAGENTA             0x744bba
#define CYAN                0x538aab
#define WHITE               0xc6cdd5
#define BRIGHT_BLACK        0x89929b
#define BRIGHT_RED          0xfa7970
#define BRIGHT_GREEN        0x7ce38b
#define BRIGHT_YELLOW       0xfaa356
#define BRIGHT_BLUE         0x77bdfb
#define BRIGHT_MAGENTA      0xcea5fb
#define BRIGHT_CYAN         0xa2d2fb
#define BRIGHT_WHITE        0xf2f2f2

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

    frontbuffer = fb->addr;
    frontbuffer_end = fb->addr + fb->width * fb->height;
    backbuffer = nullptr;
    backbuffer_end = nullptr;
    backbuffer_pos = nullptr;

    needs_render = false;

    fg = FOREGROUND;
    bg = BACKGROUND;

    state = NONE;
    param_index = 0;
    params[0] = 0;
    params[1] = 0;
    params[2] = 0;
    params[3] = 0;

    line_buffered = true;
    echo = true;

    input_cursor = 0;

    read_queue = nullptr;

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
    u64 value = ((u64)bg << 32) | bg;

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

void FramebufferTerminal::scroll()
{
    u64 value = ((u64)bg << 32) | bg;
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

void FramebufferTerminal::write(const char* buffer, usize len)
{
    const char* ptr = buffer;
    usize i = 0;

    draw_cursor(bg);

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
                params[param_index] = params[param_index] * 10 + *ptr - '0';
            else if (*ptr == ';')
            {
                if (param_index < 4)
                    param_index++;
            }
            else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z'))
            {
                for (int j = 0; j <= param_index; j++)
                    ansi_function(*ptr, params[j]);

                state = NONE;
            }

            break;
        }

        ptr++;
        i++;
    }

    draw_cursor(fg);
    cursor_ticks = 0;
}

isize FramebufferTerminal::read(char* buffer, usize len)
{
    if (line_buffered && len > input_cursor)
    {
        add_request(buffer, len);

        // this should never be reached
        return 0;
    }

    usize read = input_cursor;

    if (read > len)
        read = len;

    for (usize i = 0; i < read; i++)
    {
        if (input_buffer[i] == '\n')
        {
            read = i + 1;
            break;
        }
    }

    memcpy(buffer, input_buffer, read);
    input_cursor -= read;
    memmove(input_buffer, input_buffer + read, input_cursor);

    return read;
}

void FramebufferTerminal::ansi_function(char name, int arg)
{
    switch (name)
    {
    case 'm':

        if (arg >= 30 && arg <= 37)
            fg = colors[arg - 30];
        else if (arg >= 40 && arg <= 47)
            bg = colors[arg - 40];
        else if (arg >= 90 && arg <= 97)
            fg = colors[arg - 90 + 8];
        else if (arg >= 100 && arg <= 107)
            bg = colors[arg - 100 + 8];
        else if (arg == 0)
        {
            fg = FOREGROUND;
            bg = BACKGROUND;
        }

        break;
    }
}

void FramebufferTerminal::putchar(char ch)
{
    if (ch >= ' ')
    {
        draw_bitmap(ch);
        cursor++;
    }
    else
    {
        if (ch == '\n')
            cursor += width - cursor % width;
        else if (ch == '\r')
            cursor -= cursor % width;
        else if (ch == '\b')
        {
            if (cursor > write_cursor)
            {
                cursor--;
                draw_bitmap(' ');
            }
        }
    }

    if (cursor >= width * height)
        scroll();
}

void FramebufferTerminal::receive_char(char ch)
{
    if (ch == '\b' && line_buffered)
    {
        if (input_cursor > 0)
            input_cursor--;
    }
    else
    {
        if (input_cursor < INPUT_BUFFER_SIZE)
            input_buffer[input_cursor++] = ch;

        if (ch == '\n' && line_buffered)
            handle_requests();
    }

    if (echo)
        write(&ch, 1);
}

void FramebufferTerminal::tick()
{
    if (needs_render)
    {
        render();
        needs_render = false;
    }
    else if (cursor_ticks % 20 == 0)
        fbterm.draw_cursor(cursor_ticks % 40 ? fbterm.bg : fbterm.fg);

    cursor_ticks++;
}

void FramebufferTerminal::add_request(char* buffer, usize len)
{
    ReadRequest* rr = (ReadRequest*)kmalloc(sizeof(ReadRequest));

    rr->task = running;

    // if the read came from a user task
    // i think it's better to store the kernel mapping of the buffer
    // so we can write to it without switching the page table

    if (running->mm->pml4)
        rr->buffer = (char*)vmm.user_to_kernel(running->mm->pml4, (u64)buffer);
    else
        rr->buffer = buffer;

    rr->len = len;
    rr->read = input_cursor;
    rr->next = nullptr;

    memcpy(buffer, input_buffer, input_cursor);
    input_cursor = 0;

    if (!read_queue)
        read_queue = rr;
    else
    {
        ReadRequest* last = read_queue;

        while (last->next)
            last = last->next;

        last->next = rr;
    }

    running->sleep();
}

void FramebufferTerminal::handle_requests()
{
    ReadRequest* rr = read_queue;

    while (rr)
    {
        usize read = rr->len - rr->read;

        if (read > input_cursor)
            read = input_cursor;

        memcpy(rr->buffer + rr->read, input_buffer, read);
        rr->read += read;
        input_cursor -= read;

        memmove(input_buffer, input_buffer + read, input_cursor);

        rr->task->return_from_syscall(rr->read);

        read_queue = rr->next;
        kfree(rr);

        if (input_cursor == 0)
            break;

        rr = read_queue;
    }
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

    u8* font_ptr = font->glyph_buffer + ch * font->header->char_size;
    u32* front_ptr = frontbuffer + x + y * fb->width;
    u32* draw_ptr;

    if (backbuffer)
    {
        draw_ptr = backbuffer_pos + x + y * fb->width;

        if (draw_ptr >= backbuffer_end)
            draw_ptr -= fb->width * fb->height;
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

    //         draw_ptr[x] = (*font_ptr & (0b10000000 >> (x & 7))) ? fg : bg;
    //     }

    //     font_ptr++;
    //     draw_ptr += fb->width;
    // }

    // new alpha blended drawing
    for (y = 0; y < font->header->height; y++)
    {
        for (x = 0; x < font->header->width; x++)
            draw_ptr[x] = alpha_blend(fg, bg, font_ptr[x]);

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

void FramebufferTerminal::draw_cursor(u32 color)
{
    u32 x = (cursor % width) * font->header->width;
    u32 y = (1 + cursor / width) * font->header->height;

    u32* ptr = frontbuffer + x + (y - 2) * fb->width;

    for (y = 0; y < 2; y++)
    {
        for (x = 0; x < font->header->width; x++)
            ptr[x] = color;

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

isize fbterm_read(File* file, char* buf, usize size, usize offset)
{
    return fbterm.read(buf, size);
}

isize fbterm_write(File* file, const char* buf, usize size, usize offset)
{
    fbterm.write(buf, size);
    fbterm.write_cursor = fbterm.cursor;

    return size;
}

int fbterm_ioctl(File* file, int cmd, void* arg)
{
    if (cmd == 1)
    {
        fbterm.clear();
        return 0;
    }

    return -1;
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