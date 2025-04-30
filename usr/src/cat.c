#include <sys.h>

#define BUFFER_SIZE 4096

void cat(int fd)
{
    char buffer[BUFFER_SIZE];
    long bytes_read;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
        write(1, buffer, bytes_read);
}

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        cat(0);
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-' && argv[i][1] == '\0')
            {
                cat(0);
                continue;
            }

            int fd = open(argv[i], 0, 0);

            if (fd < 0)
            {
                write(2, "cat: error opening file\n", 24);
                continue;
            }

            cat(fd);
            close(fd);
        }
    }

    return 0;
}