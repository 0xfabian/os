#include <sys.h>

int _strlen(const char* str)
{
    int len = 0;

    while (str[len])
        len++;

    return len;
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        write(1, argv[i], _strlen(argv[i]));
        write(1, " ", 1);
    }

    write(1, "\n", 1);

    return 0;
}