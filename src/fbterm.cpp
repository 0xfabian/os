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

uint32_t colors[] =
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
uint32_t* backbuffer;
FramebufferTerminal fbterm;

void Framebuffer::init(limine_framebuffer* fb)
{
    width = fb->width;
    height = fb->height;
    addr = (uint32_t*)fb->address;
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

    size_t count = PAGE_COUNT(default_fb.width * default_fb.height * sizeof(uint32_t));
    backbuffer = (uint32_t*)((uint64_t)pfa.alloc_pages(count) | 0xffff800000000000);

    if (!backbuffer)
    {
        kprintf(WARN "Failed to allocate backbuffer\n");
        return;
    }

    uint64_t* from = (uint64_t*)fb->addr;
    uint64_t* to = (uint64_t*)backbuffer;
    uint64_t* end = (uint64_t*)(fb->addr + fb->width * fb->height);

    while (from < end)
        *to++ = *from++;
}

void FramebufferTerminal::clear()
{
    uint64_t* start = (uint64_t*)backbuffer;
    uint64_t* end = (uint64_t*)(backbuffer + fb->width * fb->height);
    uint64_t value = ((uint64_t)bg << 32) | bg;

    for (uint64_t* ptr = start; ptr < end; ptr++)
        *ptr = value;

    cursor = 0;

    render();
}

void FramebufferTerminal::scroll()
{
    uint64_t* start = (uint64_t*)backbuffer;
    uint64_t* end = (uint64_t*)(backbuffer + fb->width * (fb->height - font->header->height));
    uint64_t scroll_offset = fb->width * font->header->height / 2;
    uint64_t value = ((uint64_t)bg << 32) | bg;

    for (uint64_t* ptr = start; ptr < end; ptr++)
        *ptr = *(ptr + scroll_offset);

    for (uint64_t* ptr = end; ptr < end + scroll_offset; ptr++)
        *ptr = value;

    cursor -= width;

    render();
}

void FramebufferTerminal::write(const char* buffer, size_t len)
{
    const char* ptr = buffer;
    size_t i = 0;

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
    uint32_t x = (cursor % width) * font->header->width;
    uint32_t y = (cursor / width) * font->header->height;

    uint32_t* ptr = backbuffer + x + y * fb->width;
    uint32_t* ptr2 = fb->addr + x + y * fb->width;
    uint8_t* font_ptr = font->glyph_buffer + c * font->header->char_size;

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

    uint64_t* from = (uint64_t*)backbuffer;
    uint64_t* to = (uint64_t*)fb->addr;
    uint64_t* end = (uint64_t*)(backbuffer + fb->width * fb->height);

    while (from < end)
        *to++ = *from++;
}