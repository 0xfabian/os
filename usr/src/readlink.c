#include <user.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: %s <symlink>\n", argv[0]);
        return 1;
    }

    char buf[256];
    isize bytes = readlink(argv[1], buf, sizeof(buf) - 1);

    if (bytes < 0)
    {
        printf("%s: readlink failed\n", argv[0]);
        return 1;
    }

    buf[bytes] = 0; // null terminate the buffer
    printf("%s\n", buf);

    return 0;
}
