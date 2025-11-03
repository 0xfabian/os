#include <user.h>

#define BUF_SIZE 4096
#define COLS     16

// offset + ':' + ' ' + COLS * 3 + ' ' + COLS + '\n'
#define LINE_MAX (8 + 1 + 1 + COLS * 3 + 1 + COLS + 1)

void hexdump_line(usize offset, u8* data, usize size)
{
    printf("%08x: ", (u32)offset);

    for (usize i = 0; i < COLS; i++)
    {
        if (i < size)
            printf("%02hhx ", data[i]);
        else
            printf("   ");
    }

    u8 ascii[COLS + 1];

    for (usize i = 0; i < size; i++)
        ascii[i] = isprint(data[i]) ? data[i] : '.';

    ascii[size] = 0;

    printf(" %s\n", ascii);
}

void hexdump(int fd)
{
    // normally output is flushed after each printf call
    // we don't want that...
    auto_flush = 0;

    u8 buf[BUF_SIZE];
    isize bytes;
    u8 line[COLS];
    usize line_size = 0;
    usize offset = 0;

    while ((bytes = read(fd, buf, sizeof(buf))) > 0)
    {
        for (usize i = 0; i < bytes; i++)
        {
            line[line_size++] = buf[i];

            if (line_size == COLS)
            {
                hexdump_line(offset, line, line_size);
                offset += line_size;
                line_size = 0;
            }
        }
    }

    if (line_size)
        hexdump_line(offset, line, line_size);

    if (bytes < 0)
        printf("error while reading\n");

    flush();
}

u8 nibble_value(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';

    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;

    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;

    return 0; // invalid nibble
}

// byte should be at least 2 chars long
u8 str_to_byte(char* byte)
{
    char hi = byte[0];
    char lo = byte[1];

    return (nibble_value(hi) << 4) | nibble_value(lo);
}

void reverse_hexdump_line(char* line, usize size)
{
    u8 data[COLS];
    usize data_size = 0;
    usize start = 8 + 1 + 1; // skip offset and ': '
    usize end = start + COLS * 3;
    for (usize i = start; i < end; i += 3)
    {
        if (i >= size)
            break;

        if (line[i] == ' ')
            break;

        data[data_size++] = str_to_byte(&line[i]);
    }

    write(STDOUT_FILENO, data, data_size);
}

void reverse_hexdump(int fd)
{
    auto_flush = 0;

    u8 buf[BUF_SIZE];
    isize bytes;
    u8 line[LINE_MAX];
    usize line_size = 0;

    while ((bytes = read(fd, buf, sizeof(buf))) > 0)
    {
        for (usize i = 0; i < bytes; i++)
        {
            line[line_size++] = buf[i];

            if (line_size == LINE_MAX || buf[i] == '\n')
            {
                reverse_hexdump_line(line, line_size);
                line_size = 0;
            }
        }
    }

    if (line_size)
        reverse_hexdump_line(line, line_size);

    if (bytes < 0)
        printf("error while reading\n");

    flush();
}

int main(int argc, char** argv)
{
    bool reverse = false;
    char* filename = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-r") != 0)
            {
                printf("usage: %s [-r] [file]\n", argv[0]);
                return 1;
            }

            reverse = true;
        }
        else
        {
            filename = argv[i];
        }
    }

    int fd = STDIN_FILENO;

    if (filename)
    {
        fd = open(filename, O_RDONLY, 0);

        if (fd < 0)
        {
            printf("%s: open failed\n", argv[0]);
            return 1;
        }
    }

    if (reverse)
        reverse_hexdump(fd);
    else
        hexdump(fd);

    if (fd != STDIN_FILENO)
        close(fd);

    return 0;
}
