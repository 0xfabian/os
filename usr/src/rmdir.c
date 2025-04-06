#include <sys.h>

int strlen(const char* str)
{
    int len = 0;

    while (str[len])
        len++;

    return len;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        write(STDOUT_FILENO, "usage: rmdir <name...>\n", 23);
        return 1;
    }

    int ret = 0;

    for (int i = 1; i < argc; i++)
    {
        int err = rmdir(argv[i]);

        if (err)
        {
            write(STDOUT_FILENO, "rmdir: failed to remove `", 25);
            write(STDOUT_FILENO, argv[i], strlen(argv[i]));
            write(STDOUT_FILENO, "`\n", 2);
            ret++;
        }
    }

    return ret;
}