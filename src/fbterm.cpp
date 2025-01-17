#include <fbterm.h>

#define FOREGROUND          0xF0FFFF
#define BACKGROUND          0x192840
#define BLACK               0x1A1C1F
#define RED                 0xC50F1F
#define GREEN               0x34A116
#define YELLOW              0xF2A623
#define BLUE                0x2B5CB3
#define MAGENTA             0x8A155E
#define CYAN                0x3DBAB5
#define WHITE               0xCCCCCC
#define BRIGHT_BLACK        0x8D95A3
#define BRIGHT_RED          0xFF4A4A
#define BRIGHT_GREEN        0x89FF64
#define BRIGHT_YELLOW       0xFFEA3A
#define BRIGHT_BLUE         0x6FA3FF
#define BRIGHT_MAGENTA      0xB53083
#define BRIGHT_CYAN         0x79F9DE
#define BRIGHT_WHITE        0xF2F2F2

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
FramebufferTerminal fbterm;

void Framebuffer::init(limine_framebuffer* fb)
{
    width = fb->width;
    height = fb->height;
    addr = (uint32_t*)fb->address;
}

void FramebufferTerminal::init()
{
    fb = &default_fb;
    font = &zap_light20;

    width = fb->width / font->header->width;
    height = fb->height / font->header->height;
    cursor = 0;

    fg = FOREGROUND;
    bg = BACKGROUND;

    clear();
}

void FramebufferTerminal::clear()
{
    uint64_t* start = (uint64_t*)fb->addr;
    uint64_t* end = (uint64_t*)(fb->addr + fb->width * fb->height);
    uint64_t value = ((uint64_t)bg << 32) | bg;

    for (uint64_t* ptr = start; ptr < end; ptr++)
        *ptr = value;

    cursor = 0;
}

void FramebufferTerminal::scroll()
{
    uint64_t* start = (uint64_t*)fb->addr;
    uint64_t* end = (uint64_t*)(fb->addr + fb->width * (fb->height - font->header->height));
    uint64_t scroll_offset = fb->width * font->header->height / 2;
    uint64_t value = ((uint64_t)bg << 32) | bg;

    for (uint64_t* ptr = start; ptr < end; ptr++)
        *ptr = *(ptr + scroll_offset);

    for (uint64_t* ptr = end; ptr < end + scroll_offset; ptr++)
        *ptr = value;

    cursor -= width;
}

void FramebufferTerminal::write(const char* buffer, size_t len)
{
    const char* ptr = buffer;
    size_t i = 0;

    enum EscapeState
    {
        NONE,
        EXPECT_CSI,
        TAKE_PARAMS,
    };

    EscapeState state = NONE;
    int param_index = 0;
    int params[4] = { 0, 0, 0, 0 };

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
            else if (*ptr >= 'a' && *ptr <= 'z' || *ptr >= 'A' && *ptr <= 'Z')
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
    }

    if (cursor >= width * height)
        scroll();
}

void FramebufferTerminal::draw_bitmap(uint8_t chr)
{
    uint32_t x = (cursor % width) * font->header->width;
    uint32_t y = (cursor / width) * font->header->height;

    uint32_t* ptr = fb->addr + x + y * fb->width;
    uint8_t* font_ptr = font->glyph_buffer + chr * font->header->char_size;

    for (y = 0; y < font->header->height; y++)
    {
        for (x = 0; x < font->header->width; x++)
        {
            if (x == 8)
                font_ptr++;

            ptr[x] = (*font_ptr & (0b10000000 >> (x & 7))) ? fg : bg;
        }

        font_ptr++;
        ptr += fb->width;
    }
}