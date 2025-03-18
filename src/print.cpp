#include <print.h>
#include <task.h>

#define BUFFER_SIZE 256

char buf_start[BUFFER_SIZE];
char* buf_end;

inline void append_char(char c)
{
    *buf_end++ = c;
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

void append_address(void* addr)
{
    u64 num = (u64)addr;

    num &= 0x0000ffffffffffff;

    if ((num >> 47) & 1)
    {
        num |= 0xffff000000000000;
        append_string("0x");
    }
    else
        append_string("\e[90m0x\e[m");

    static const char* hex = "0123456789abcdef";

    for (int i = 60; i >= 0; i -= 4)
    {
        bool is_pad = num >> i == 0 && i != 0;

        if (is_pad)
            append_string("\e[90m");

        append_char(hex[(num >> i) & 0x0f]);

        if (is_pad)
            append_string("\e[m");
    }
}

void append_binary(u64 num, int size)
{
    for (int i = size * 8 - 1; i >= 0; i--)
        append_char((num >> i) & 1 ? '1' : '0');
}

int parse_width_mod(const char*& ptr)
{
    int width = 0;
    bool negative = false;

    if (*ptr == '-')
    {
        negative = true;
        ptr++;
    }

    while (*ptr >= '0' && *ptr <= '9')
    {
        width = width * 10 + (*ptr - '0');
        ptr++;
    }

    return negative ? -width : width;
}

int parse_size_mod(const char*& ptr)
{
    int size = 2;

    if (*ptr == 'l')
    {
        size++;
        ptr++;
    }
    else if (*ptr == 'h')
    {
        size--;
        ptr++;

        if (*ptr == 'h')
        {
            size--;
            ptr++;
        }
    }

    return size;
}

void kprintf(const char* fmt, ...)
{
    u64 rflags = read_rflags();
    cli();

    buf_end = buf_start;
    char* arg_start = nullptr;

    va_list args;
    va_start(args, fmt);

    for (const char* ptr = fmt; *ptr; ptr++)
    {
        if (*ptr != '%')
        {
            append_char(*ptr);
            continue;
        }

        ptr++;

        int width = *ptr == '*' ? (ptr++, va_arg(args, int)) : parse_width_mod(ptr);
        int size = parse_size_mod(ptr);

        arg_start = buf_end;

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
        case 'b':
        {
            switch (size)
            {
            case 0: append_binary(va_arg(args, unsigned int), 1);               break;
            case 1: append_binary(va_arg(args, unsigned int), 2);               break;
            case 2: append_binary(va_arg(args, unsigned int), 4);               break;
            case 3: append_binary(va_arg(args, unsigned long long), 8);         break;
            }

            break;
        }
        case 'f': append_string(va_arg(args, int) ? "true" : "false");          break;
        case 'p':
        {
            append_char('0');
            append_char('x');
            append_hex((u64)va_arg(args, void*), 8);

            break;
        }
        case 'a': append_address(va_arg(args, void*));                          break;
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

        usize len = buf_end - arg_start;
        int padding = (width < 0 ? -width : width) - len;

        if (padding <= 0)
            continue;

        if (width > 0)
        {
            memmove(arg_start + padding, arg_start, len);
            memset(arg_start, ' ', padding);
        }
        else
            memset(buf_end, ' ', padding);

        buf_end += padding;
    }

    va_end(args);

    fbterm.write(buf_start, buf_end - buf_start);

    if (rflags & 0x200)
        sti();
}

void hexdump(void* data, usize len)
{
    u8* ptr = (u8*)data;

    for (usize i = 0; i < len; i++)
    {
        if (i % 16 == 0)
            kprintf("\e[92m%p\e[m: ", ptr + i);

        kprintf("%hhx ", ptr[i]);

        if (i % 16 == 15)
            kprintf("\n");
    }
}

void panic(const char* msg)
{
    if (fbterm.cursor % fbterm.width)
        kprintf("\n");

    kprintf(PANIC "%s\n", msg);
    idle();
}