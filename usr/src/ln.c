#include <user.h>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("usage: ln <target> <name>\n");
        return 1;
    }

    return symlink(argv[1], argv[2]);
}
