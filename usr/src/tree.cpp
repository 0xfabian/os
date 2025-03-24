#include <user.h>

#define BASENAME_BUF_SIZE   256

char basename_buf[BASENAME_BUF_SIZE];

const char* basename(const char* path)
{
    if (*path == 0)
        return nullptr;

    strcpy(basename_buf, path);

    char* end = basename_buf + strlen(basename_buf) - 1;

    while (*end == '/')
    {
        if (end == basename_buf)
            return nullptr;

        *end-- = '\0';
    }

    while (end >= basename_buf && *end != '/')
        end--;

    return end + 1;
}

struct Dirent
{
    unsigned long ino;
    unsigned int type;
    char name[32];
};

void tree(const char* path, const char* prefix = "", bool last_dir = true)
{
    int fd = open(path, O_DIRECTORY, 0);

    printf("\e[90m%s", prefix);

    if (fd >= 0)
    {
        const char* dirname = basename(path);

        if (!dirname)
            dirname = "/";

        printf("\e[94m%s\e[m\n", dirname);
    }
    else
    {
        printf("\e[m%s\n", basename(path));
        return;
    }

    char entries[100][32];
    int count = 0;

    struct Dirent dent;

    while (getdents(fd, &dent, sizeof(dent)) > 0)
        if (dent.name[0] != '.')
            strcpy(entries[count++], dent.name);

    close(fd);

    for (int i = 0; i < count; i++)
    {
        bool last_ent = (i == count - 1);

        char new_prefix[256];
        strcpy(new_prefix, prefix);

        if (strlen(new_prefix) >= 1)
        {
            new_prefix[strlen(new_prefix) - 2] = 0;

            if (last_dir)
                strcat(new_prefix, "  ");
            else
                strcat(new_prefix, "\xb3 ");
        }

        if (last_ent)
            strcat(new_prefix, "\xc0\xc4");
        else
            strcat(new_prefix, "\xc3\xc4");

        char next_path[256];
        strcpy(next_path, path);
        strcat(next_path, "/");
        strcat(next_path, entries[i]);

        tree(next_path, new_prefix, last_ent);
    }
}

int main(int argc, char** argv)
{
    tree((argc > 1) ? argv[1] : ".");

    return 0;
}