#include <fbterm.h>

#define FOREGROUND          0xf0ffff
#define BACKGROUND          0x192840
#define BLACK               0x1a1c1f
#define RED                 0xc50f1f
#define GREEN               0x34a116
#define YELLOW              0xf2a623
#define BLUE                0x2b5cb3
#define MAGENTA             0x8a155e
#define CYAN                0x3dbab5
#define WHITE               0xcccccc
#define BRIGHT_BLACK        0x8d95a3
#define BRIGHT_RED          0xff4a4a
#define BRIGHT_GREEN        0x89ff64
#define BRIGHT_YELLOW       0xffea3a
#define BRIGHT_BLUE         0x6fa3ff
#define BRIGHT_MAGENTA      0xb53083
#define BRIGHT_CYAN         0x79f9de
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
    backbuffer = default_fb.addr;

    fb = &default_fb;
    font = &zap_light20;

    width = fb->width / font->header->width;
    height = fb->height / font->header->height;
    cursor = 0;

    fg = FOREGROUND;
    bg = BACKGROUND;

    state = NONE;
    param_index = 0;
    params[0] = 0;
    params[1] = 0;
    params[2] = 0;
    params[3] = 0;

    clear();

    kprintf(INFO "Framebuffer terminal initialized (%ux%u)\n", width, height);
}

void FramebufferTerminal::enable_backbuffer()
{
    kprintf(INFO "Enabling backbuffer...\n");

    usize count = PAGE_COUNT(default_fb.width * default_fb.height * sizeof(u32));
    backbuffer = (u32*)vmm.alloc_pages(count, PE_WRITE);

    if (!backbuffer)
    {
        backbuffer = fb->addr;

        kprintf(WARN "Failed to allocate backbuffer\n");
        return;
    }

    // this is very slow because we are reading from the framebuffer
    // but i don't now any better way to sync the two buffers

    u64* from = (u64*)fb->addr;
    u64* to = (u64*)backbuffer;
    u64* end = (u64*)(fb->addr + fb->width * fb->height);

    while (from < end)
        *to++ = *from++;
}

void FramebufferTerminal::clear()
{
    u64* start = (u64*)backbuffer;
    u64* end = (u64*)(backbuffer + fb->width * fb->height);
    u64 value = ((u64)bg << 32) | bg;

    for (u64* ptr = start; ptr < end; ptr++)
        *ptr = value;

    cursor = 0;

    render();
}

void FramebufferTerminal::scroll()
{
    u64* start = (u64*)backbuffer;
    u64* end = (u64*)(backbuffer + fb->width * (fb->height - font->header->height));
    u64 scroll_offset = fb->width * font->header->height / 2;
    u64 value = ((u64)bg << 32) | bg;

    for (u64* ptr = start; ptr < end; ptr++)
        *ptr = *(ptr + scroll_offset);

    for (u64* ptr = end; ptr < end + scroll_offset; ptr++)
        *ptr = value;

    cursor -= width;

    render();
}

void FramebufferTerminal::write(const char* buffer, usize len)
{
    const char* ptr = buffer;
    usize i = 0;

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

void FramebufferTerminal::putchar(char c)
{
    if (c >= ' ')
    {
        draw_bitmap(c);
        cursor++;
    }
    else
    {
        if (c == '\n')
            cursor += width - cursor % width;
        else if (c == '\r')
            cursor -= cursor % width;
    }

    if (cursor >= width * height)
        scroll();
}

void FramebufferTerminal::draw_bitmap(char c)
{
    u32 x = (cursor % width) * font->header->width;
    u32 y = (cursor / width) * font->header->height;

    u32* ptr = backbuffer + x + y * fb->width;
    u32* ptr2 = fb->addr + x + y * fb->width;
    u8* font_ptr = font->glyph_buffer + c * font->header->char_size;

    for (y = 0; y < font->header->height; y++)
    {
        for (x = 0; x < font->header->width; x++)
        {
            if (x == 8)
                font_ptr++;

            ptr[x] = (*font_ptr & (0b10000000 >> (x & 7))) ? fg : bg;
            ptr2[x] = (*font_ptr & (0b10000000 >> (x & 7))) ? fg : bg;
        }

        font_ptr++;
        ptr += fb->width;
        ptr2 += fb->width;
    }
}

void FramebufferTerminal::render()
{
    if (backbuffer == fb->addr)
        return;

    u64* from = (u64*)backbuffer;
    u64* to = (u64*)fb->addr;
    u64* end = (u64*)(backbuffer + fb->width * fb->height);

    while (from < end)
        *to++ = *from++;
}