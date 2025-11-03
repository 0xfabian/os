#include <user.h>

#define BUF_SIZE 4096
char buf[BUF_SIZE];
char line[BUF_SIZE];
int total_matches = 0;

char* match_pattern(char* str, char* pattern, int pat_len)
{
    int str_len = strlen(str);

    if (pat_len > str_len)
        return 0;

    for (int i = 0; i <= str_len - pat_len; i++)
    {
        int match = 1;

        for (int j = 0; j < pat_len; j++)
        {
            if (str[i + j] != pattern[j])
            {
                match = 0;
                break;
            }
        }

        if (match)
            return str + i;
    }

    return 0;
}

void do_grep_on_line(char* pattern, int pat_len)
{
    char* ptr = line;

    while (1)
    {
        char* begin = ptr;
        ptr = match_pattern(ptr, pattern, pat_len);

        if (!ptr)
        {
            if (begin != line)
                write(1, begin, strlen(begin));

            break;
        }
        else
        {
            write(1, begin, ptr - begin);

            if (isatty(1))
            {
                write(1, "\e[91m", 5);
                write(1, ptr, pat_len);
                write(1, "\e[39m", 5);
            }
            else
                write(1, ptr, pat_len);

            total_matches++;

            ptr += pat_len;
        }
    }
}

int grep(int fd, char* pattern)
{
    int pat_len = strlen(pattern);
    int bytes_read = 0;
    int line_pos = 0;

    while ((bytes_read = read(fd, buf, BUF_SIZE)) > 0)
    {
        for (int i = 0; i < bytes_read; i++)
        {
            line[line_pos++] = buf[i];

            if (buf[i] == '\n')
            {
                line[line_pos] = 0;
                line_pos = 0;

                do_grep_on_line(pattern, pat_len);
            }
        }
    }

    if (bytes_read == 0 && line_pos > 0)
    {
        line[line_pos] = 0;
        do_grep_on_line(pattern, pat_len);
    }

    return total_matches > 0 ? 0 : 1;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        write(2, "usage: grep <pattern> [file...]\n", 32);
        return 1;
    }

    int fd = 0;

    if (argc > 2)
    {
        fd = open(argv[2], O_RDONLY, 0);

        if (fd < 0)
        {
            write(2, "grep: open failed\n", 18);
            return 1;
        }
    }

    int ret = grep(fd, argv[1]);

    if (fd != 0)
        close(fd);

    return ret;
}
