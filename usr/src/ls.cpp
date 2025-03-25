#include <user.h>

struct Dirent
{
    unsigned long ino;
    unsigned int type;
    char name[32];
};

#define IT_DIR 0x4000

void ls(const char* path)
{
    int fd = open(path, O_DIRECTORY, 0);

    if (fd < 0)
    {
        write(1, "ls: open failed\n", 16);
        return;
    }

    auto_flush = false;

    Dirent dents[64];
    long bytes_read = 0;

    while ((bytes_read = getdents(fd, dents, sizeof(dents))) > 0)
    {
        int count = bytes_read / sizeof(Dirent);

        for (Dirent* dent = dents; dent < dents + count; dent++)
            printf((dent->type & IT_DIR) == IT_DIR ? "\e[94m%s\e[m " : "%s ", dent->name);
    }

    printf("\n");

    flush();

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