#include <print.h>

#define BUFFER_SIZE 256
char buf[BUFFER_SIZE];
int buf_index = 0;

void write_buf()
{
    fbterm.write(buf, buf_index);
    buf_index = 0;
}

void append_char(char c)
{
    if (buf_index == BUFFER_SIZE)
        write_buf();

    buf[buf_index++] = c;
}

void append_string(const char* str)
{
    while (*str)
        append_char(*str++);
}

void append_uint(uint64_t num)
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

void append_int(int64_t num)
{
    if (num < 0)
    {
        append_char('-');
        num = -num;
    }

    append_uint(num);
}

void append_hex(uint64_t num, int size)
{
    static const char* hex = "0123456789abcdef";

    for (int i = size * 8 - 4; i >= 0; i -= 4)
        append_char(hex[(num >> i) & 0x0f]);
}

void append_binary(uint64_t num, int size)
{
    for (int i = size * 8 - 1; i >= 0; i--)
        append_char((num >> i) & 1 ? '1' : '0');
}

void kprintf(const char* fmt, ...)
{
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

        int size_mod = 2;

        if (*ptr == 'l')
        {
            size_mod = 3;
            ptr++;
        }
        else if (*ptr == 'h')
        {
            size_mod = 1;
            ptr++;

            if (*ptr == 'h')
            {
                size_mod = 0;
                ptr++;
            }
        }

        switch (*ptr)
        {
        case 'd':
        case 'i':
        {
            switch (size_mod)
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
            switch (size_mod)
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
            switch (size_mod)
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
            switch (size_mod)
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
            append_hex((uint64_t)va_arg(args, void*), 8);

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
    }

    write_buf();
    va_end(args);
}