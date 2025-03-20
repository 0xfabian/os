#include <sys.h>

struct Dirent
{
    unsigned long ino;
    unsigned int type;
    char name[32];
};

#define IT_DIR 0x4000

int strlen(const char* str)
{
    int len = 0;

    while (str[len])
        len++;

    return len;
}

void ls(const char* path)
{
    int fd = open(path, 0);

    if (fd < 0)
    {
        write(1, "ls: open failed\n", 16);
        return;
    }

    struct Dirent dent;
    while (getdents(fd, (char*)&dent, sizeof(struct Dirent)) > 0)
    {
        if ((dent.type & IT_DIR) == IT_DIR)
            write(1, "\e[94m", 5);

        write(1, dent.name, strlen(dent.name));

        if ((dent.type & IT_DIR) == IT_DIR)
            write(1, "\e[m", 3);

        write(1, " ", 1);
    }

    write(1, "\n", 1);

    close(fd);
}

int main(int argc, char** argv)
{
    if (argc < 2)
        ls(".");
    else
        ls(argv[1]);

    return 0;
}