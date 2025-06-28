#include <sys.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        write(STDOUT_FILENO, "usage: mkdir <name>\n", 20);
        return 1;
    }

    int err = mkdir(argv[1]);

    if (err)
    {
        write(STDOUT_FILENO, "mkdir: failed to create directory\n", 34);
        return 1;
    }

    return 0;
}
