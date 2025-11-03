#include <sys.h>

u32 parse_dev(char* str)
{
    u32 ret = 0;

    while (*str)
    {
        ret = ret * 16 + (*str > 'a' ? *str - 'a' + 10 : *str - '0');
        str++;
    }

    return ret;
}

int strcmp(const char* str1, const char* str2)
{
    while (*str1 && *str1 == *str2)
    {
        str1++;
        str2++;
    }

    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        write(STDOUT_FILENO, "usage: mknod <name> <type> <dev>\n", 33);
        return 1;
    }

    u32 type = 0;

    if (strcmp(argv[2], "b") == 0)
        type = IT_BDEV;
    else if (strcmp(argv[2], "c") == 0)
        type = IT_CDEV;
    else
    {
        write(STDOUT_FILENO, "mknod: invalid type\n", 20);
        return 1;
    }

    int err = mknod(argv[1], type, parse_dev(argv[3]));

    if (err)
    {
        write(STDOUT_FILENO, "mknod: failed to create node\n", 29);
        return 1;
    }

    return 0;
}
