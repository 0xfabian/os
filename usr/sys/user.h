#pragma once

#include <stdarg.h>
#include <sys.h>

void* memcpy(void* dest, const void* src, usize n)
{
    u8* pdest = (u8*)dest;
    const u8* psrc = (const u8*)src;

    for (usize i = 0; i < n; i++)
        pdest[i] = psrc[i];

    return dest;
}

void* memset(void* s, int c, usize n)
{
    u8* p = (u8*)s;

    for (usize i = 0; i < n; i++)
        p[i] = (u8)c;

    return s;
}

void* memmove(void* dest, const void* src, usize n)
{
    u8* pdest = (u8*)dest;
    const u8* psrc = (const u8*)src;

    if (src > dest)
    {
        for (usize i = 0; i < n; i++)
            pdest[i] = psrc[i];
    }
    else if (src < dest)
    {
        for (usize i = n; i > 0; i--)
            pdest[i - 1] = psrc[i - 1];
    }

    return dest;
}

usize strlen(const char* str)
{
    usize len = 0;

    while (str[len])
        len++;

    return len;
}

char* strcpy(char* dest, const char* src)
{
    usize len = strlen(src);

    memcpy((void*)dest, (void*)src, len + 1);

    return dest;
}

char* strcat(char* dest, const char* src)
{
    usize len = strlen(dest);

    strcpy(dest + len, src);

    return dest;
}

int strcmp(const char* str1, const char* str2)
{
    while (*str1 && *str1 == *str2)
    {
        str1++;
        str2++;
    }

    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* strchr(const char* str, int c)
{
    while (*str)
    {
        if (*str == c)
            return (char*)str;

        str++;
    }

    return 0;
}

char* strrchr(const char* str, int c)
{
    const char* last = 0;

    while (*str)
    {
        if (*str == c)
            last = str;

        str++;
    }

    return (char*)last;
}

int isdigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

int isxdigit(char ch)
{
    return isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

int isalpha(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

int isalnum(char ch)
{
    return isalpha(ch) || isdigit(ch);
}

int isprint(char ch)
{
    return ch >= ' ' && ch <= '~';
}

int isspace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v';
}

#define BUFFER_SIZE 4096

// this is used when the width is specified
// so the thing that is being formatted should not overflow it
//
// it's also VERY IMPORTANT to declare this before buf_start
// since otherwise append_ptr >= buf_start + BUFFER_SIZE
// which will cause a flush, resetting append_ptr
char temp_buf[256];

char buf_start[BUFFER_SIZE];
char* append_ptr = buf_start;

void flush()
{
    write(1, buf_start, append_ptr - buf_start);
    append_ptr = buf_start;
}

inline void append_char(char c)
{
    if (append_ptr >= buf_start + BUFFER_SIZE)
        flush();

    *append_ptr++ = c;
}

void append_string(const char* str)
{
    while (*str)
        append_char(*str++);
}

void append_uint(u64 num)
{
    if (num == 0)
    {
        append_char('0');
        return;
    }

    char temp[20];
    int i = 0;

    while (num)
    {
        temp[i++] = num % 10 + '0';
        num /= 10;
    }

    while (i)
        append_char(temp[--i]);
}

void append_int(i64 num)
{
    if (num < 0)
    {
        append_char('-');
        num = -num;
    }

    append_uint(num);
}

void append_hex(u64 num, int size)
{
    static const char* hex = "0123456789abcdef";

    for (int i = size * 8 - 4; i >= 0; i -= 4)
        append_char(hex[(num >> i) & 0x0f]);
}

int parse_width_mod(const char** ptr)
{
    int width = 0;
    int negative = 0;

    if (*(*ptr) == '-')
    {
        negative = 1;
        (*ptr)++;
    }

    while (*(*ptr) >= '0' && *(*ptr) <= '9')
    {
        width = width * 10 + (*(*ptr) - '0');
        (*ptr)++;
    }

    return negative ? -width : width;
}

int parse_size_mod(const char** ptr)
{
    int size = 2;

    if (*(*ptr) == 'l')
    {
        size++;
        (*ptr)++;
    }
    else if (*(*ptr) == 'h')
    {
        size--;
        (*ptr)++;

        if (*(*ptr) == 'h')
        {
            size--;
            (*ptr)++;
        }
    }

    return size;
}

int auto_flush = 1;
void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char* arg_start = 0;

    for (const char* ptr = fmt; *ptr; ptr++)
    {
        if (*ptr != '%')
        {
            append_char(*ptr);
            continue;
        }

        ptr++;

        int width = *ptr == '*' ? (ptr++, va_arg(args, int)) : parse_width_mod(&ptr);
        int size = parse_size_mod(&ptr);

        arg_start = append_ptr;

        if (width)
            append_ptr = temp_buf;

        switch (*ptr)
        {
        case 'd':
        case 'i':
        {
            switch (size)
            {
            case 0: append_int((char)va_arg(args, int));                        break;
            case 1: append_int((short)va_arg(args, int));                       break;
            case 2: append_int(va_arg(args, int));                              break;
            case 3: append_int(va_arg(args, long long));                        break;
            }

            break;
        }
        case 'u':
        {
            switch (size)
            {
            case 0: append_uint((unsigned char)va_arg(args, unsigned int));     break;
            case 1: append_uint((unsigned short)va_arg(args, unsigned int));    break;
            case 2: append_uint(va_arg(args, unsigned int));                    break;
            case 3: append_uint(va_arg(args, unsigned long long));              break;
            }

            break;
        }
        case 'x':
        {
            switch (size)
            {
            case 0: append_hex(va_arg(args, unsigned int), 1);                  break;
            case 1: append_hex(va_arg(args, unsigned int), 2);                  break;
            case 2: append_hex(va_arg(args, unsigned int), 4);                  break;
            case 3: append_hex(va_arg(args, unsigned long long), 8);            break;
            }

            break;
        }
        case 'p':
        {
            append_char('0');
            append_char('x');
            append_hex((u64)va_arg(args, void*), 8);

            break;
        }
        case 'c': append_char(va_arg(args, int));                               break;
        case 's':
        {
            char* str = va_arg(args, char*);
            append_string(str ? str : "(null)");

            break;
        }
        case '%': append_char('%');                                             break;
        }

        if (!width)
            continue;

        // null terminate the temp buffer so we can use it as a string
        *append_ptr = 0;
        int len = append_ptr - temp_buf;

        // restore the append_ptr to point into the main buffer
        append_ptr = arg_start;

        // left justify
        if (width < 0)
        {
            append_string(temp_buf);

            for (int i = len; i < -width; i++)
                append_char(' ');
        }
        else
        {
            for (int i = len; i < width; i++)
                append_char(' ');

            append_string(temp_buf);
        }
    }

    if (auto_flush)
        flush();

    va_end(args);
}