#include <sys.h>

#define BUFFER_SIZE 4096

int cat(int fd)
{
    char buffer[BUFFER_SIZE];
    isize bytes_read;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        isize bytes_written = 0;
        while (bytes_written < bytes_read)
        {
            isize ret = write(1, buffer + bytes_written, bytes_read - bytes_written);

            if (ret < 0)
            {
                write(2, "cat: error while writing\n", 25);
                return 1;
            }

            bytes_written += ret;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 1)
        return cat(0);

    int ret = 0;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' && argv[i][1] == '\0')
        {
            ret += cat(0);
            continue;
        }

        int fd = open(argv[i], 0, 0);

        if (fd < 0)
        {
            write(2, "cat: error opening file\n", 24);
            continue;
        }

        ret += cat(fd);
        close(fd);
    }

    return ret;
}