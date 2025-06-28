#pragma once

#include <sys.h>

#define CTRL(x) ((x) - 'A' + 1)

enum Key
{
    TAB = '\t',
    ENTER = '\n',
    ESC = '\e',
    BACKSPACE = 127,
    DELETE = 1000,

    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    HOME,
    END,

    SHIFT_ARROW_LEFT,
    SHIFT_ARROW_RIGHT,
    SHIFT_ARROW_UP,
    SHIFT_ARROW_DOWN,
    SHIFT_HOME,
    SHIFT_END,

    CTRL_Q,
    CTRL_S,
    CTRL_A,
    CTRL_X,
    CTRL_L
};

int term_fd = -1;

static inline int read_char(char* ch)
{
    return read(term_fd, ch, 1) == 1;
}

static inline int isdigit(char c)
{
    return c >= '0' && c <= '9';
}

int get_key()
{
    char c;

    if (!read_char(&c))
        return 0;

    if (c == ESC)
    {
        char seq[5];

        if (!read_char(seq))
            return ESC;

        if (seq[0] != '[')
            return seq[0];

        if (!read_char(seq + 1))
            return ESC;

        if (isdigit(seq[1]))
        {
            if (!read_char(seq + 2))
                return ESC;

            if (seq[2] == '~' && seq[1] == '3')
                return DELETE;

            if (seq[1] == '1')
            {
                if (!read_char(seq + 3) || !read_char(seq + 4))
                    return ESC;

                if (seq[2] == ';' && seq[3] == '2')
                {
                    switch (seq[4])
                    {
                    case 'A': return SHIFT_ARROW_UP;
                    case 'B': return SHIFT_ARROW_DOWN;
                    case 'C': return SHIFT_ARROW_RIGHT;
                    case 'D': return SHIFT_ARROW_LEFT;
                    case 'H': return SHIFT_HOME;
                    case 'F': return SHIFT_END;
                    }
                }
            }
        }
        else
        {
            switch (seq[1])
            {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            case 'H': return HOME;
            case 'F': return END;
            }
        }

        return seq[1];
    }
    else if (c == CTRL('A'))    return CTRL_A;
    else if (c == CTRL('S'))    return CTRL_S;
    else if (c == CTRL('Q'))    return CTRL_Q;
    else if (c == CTRL('X'))    return CTRL_X;
    else if (c == CTRL('L'))    return CTRL_L;
    else                return c;
}

static inline void hide_cursor()
{
    write(term_fd, "\e[25l", 5);
}

static inline void show_cursor()
{
    write(term_fd, "\e[25h", 5);
}

int num2str(int num, char* buf)
{
    char temp[20];
    int len = 0;

    do
    {
        temp[len++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    for (int i = 0; i < len; i++)
        buf[i] = temp[len - i - 1];

    return len;
}

static inline void set_cursor(int line, int col)
{
    char buf[64];
    int len = 0;

    buf[len++] = '\e';
    buf[len++] = '[';
    len += num2str(line + 1, buf + len);
    buf[len++] = ';';
    len += num2str(col + 1, buf + len);
    buf[len++] = 'H';

    write(term_fd, buf, len);
}

static inline void clear()
{
    ioctl(term_fd, FBTERM_CLEAR, 0);
}

int get_height()
{
    int height;
    ioctl(term_fd, FBTERM_GET_HEIGHT, &height);

    return height;
}

int get_width()
{
    int width;
    ioctl(term_fd, FBTERM_GET_WIDTH, &width);

    return width;
}

int term_init()
{
    term_fd = open("/dev/fbterm", O_RDWR, 0);

    if (term_fd < 0)
        return -1;

    ioctl(term_fd, FBTERM_ECHO_OFF, 0);
    ioctl(term_fd, FBTERM_LB_OFF, 0);

    hide_cursor();

    return 0;
}

void term_exit()
{
    ioctl(term_fd, FBTERM_ECHO_ON, 0);
    ioctl(term_fd, FBTERM_LB_ON, 0);

    show_cursor();

    close(term_fd);
}

usize strlen(const char* str)
{
    usize len = 0;

    while (str[len])
        len++;

    return len;
}

int strcmp(const char* a, const char* b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }

    return *a - *b;
}

char* strcpy(char* dest, const char* src)
{
    char* ret = dest;

    while (*src)
    {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = 0;

    return ret;
}

char* strcat(char* dest, const char* src)
{
    char* ret = dest;

    while (*dest)
        dest++;

    while (*src)
    {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = 0;

    return ret;
}

char* strchr(char* str, char ch)
{
    char* ret = str;

    while (*str)
    {
        if (*str == ch)
            return str;

        str++;
    }

    return 0;
}

void print(int fd, char* str)
{
    usize len = strlen(str);
    write(fd, str, len);
}

void println(int fd, char* str)
{
    usize len = strlen(str);
    str[len] = '\n';
    write(fd, str, len + 1);
    str[len] = 0;
}
