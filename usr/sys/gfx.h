#pragma once

#include <sys.h>

int fb_fd;
u32 width, height;
u32* fb;

int gfx_init()
{
    fb_fd = open("/dev/fb", O_RDWR, 0);

    if (fb < 0)
        return -1;

    ioctl(fb_fd, FB_GET_WIDTH, &width);
    ioctl(fb_fd, FB_GET_HEIGHT, &height);

    u64 brk_start = brk(0);
    fb = (u32*)brk_start;
    brk(fb + width * height);

    write(STDIN_FILENO, "\e[25l", 5);
    ioctl(STDIN_FILENO, FBTERM_ECHO_OFF, 0);
    ioctl(STDIN_FILENO, FBTERM_LB_OFF, 0);

    return 0;
}

void gfx_exit()
{
    ioctl(STDIN_FILENO, FBTERM_LB_ON, 0);
    ioctl(STDIN_FILENO, FBTERM_ECHO_ON, 0);
    write(STDIN_FILENO, "\e[25h", 5);
    ioctl(STDIN_FILENO, FBTERM_CLEAR, 0);

    close(fb_fd);
}

void blit()
{
    pwrite(fb_fd, fb, width * height * sizeof(u32), 0);
}

void clear(u32 color)
{
    for (u32 i = 0; i < width * height; i++)
        fb[i] = color;
}

#define ABS(x) ((x) < 0 ? -(x) : (x))

void draw_line(u32 x1, u32 y1, u32 x2, u32 y2, u32 color)
{
    i32 dx = ABS((i32)x2 - (i32)x1);
    i32 dy = -ABS((i32)y2 - (i32)y1);
    i32 sx = x1 < x2 ? 1 : -1;
    i32 sy = y1 < y2 ? 1 : -1;
    i32 err = dx + dy;

    while (1)
    {
        u64 index = y1 * width + x1;

        if (index >= 0 && index < width * height)
            fb[index] = color;

        if (x1 == x2 && y1 == y2)
            break;

        i32 e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x1 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

int getch()
{
    char c = 0;
    read(STDIN_FILENO, &c, 1);

    return c;
}
