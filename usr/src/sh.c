#include <sys.h>

int _strlen(const char* str)
{
    int len = 0;

    while (str[len])
        len++;

    return len;
}

int strcmp(const char* a, const char* b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }

    return *a - *b;
}

int find(const char* str, char ch)
{
    for (int i = 0; str[i]; i++)
        if (str[i] == ch)
            return i;

    return -1;
}

int exists(const char* path)
{
    int fd = open(path, 0);

    if (fd < 0)
        return 0;

    close(fd);

    return 1;
}

#define BUF_SIZE    1024
#define CWD_SIZE    256
#define ARG_MAX     16

char buf[BUF_SIZE];
char cwd[CWD_SIZE];

char* argv[ARG_MAX];
int argc;

void prepare_argv()
{
    argc = 0;
    char* ptr = buf;

    if (*ptr == 0)
        return;

    // skip leading spaces
    if (*ptr == ' ')
    {
        while (*ptr == ' ')
            ptr++;
            
        // line full of spaces
        if (*ptr == 0)
            return;
    }

    while (1)
    {
        argv[argc++] = ptr;

        while (*ptr && *ptr != ' ')
            ptr++;

        if (*ptr == 0)
            break;

        *ptr = 0;
        ptr++;

        if (*ptr == ' ')
        {
            while (*ptr == ' ')
                ptr++;

            if (*ptr == 0)
                break;
        }
    }

    argv[argc] = 0;
}

const char* paths[] =
{
    "/mnt/",
    "/"
};

char full_path[256];

void do_exec(const char* path)
{
    if (!exists(path))
        return;

    execve(path, argv, 0);
    write(1, "sh: execve failed\n", 18);
    exit(1);
}

int main()
{
    int fd = open("/dev/fbterm", 0);
    dup(fd);
    dup(fd);

    while(1)
    {
        getcwd(cwd, CWD_SIZE);

        for (int i = 0; cwd[i]; i++)
        {
            if (cwd[i] == '/')
                write(1, "\e[91m/\e[94m", 11);
            else
                write(1, cwd + i, 1);
        }

        write(1, "\e[91m>\e[m ", 10);

        long bytes = read(0, buf, BUF_SIZE - 1);

        if (bytes < 0)
            continue;

        // exit on eof
        if (bytes == 0)
        {
            write(1, "exit\n", 5);
            exit(0);
        }

        if (buf[bytes - 1] == '\n')
            buf[bytes - 1] = 0;
        else
        {
            buf[bytes] = 0;
            write(1, "\n", 1);
        }

        prepare_argv();

        if (argc == 0)
            continue;

        if (strcmp(argv[0], "exit") == 0)
            exit(0);

        if (strcmp(argv[0], "cd") == 0)
        {
            if (argc > 1 && chdir(argv[1]) < 0)
                write(1, "cd: no such directory\n", 22);

            continue;
        }

        int pid = fork();

        if (pid == 0)
        {
            if (find(argv[0], '/') >= 0)
            {
                do_exec(argv[0]);
            }
            else
            {
                for (int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
                {
                    int j = 0;

                    for (int k = 0; paths[i][k]; k++)
                        full_path[j++] = paths[i][k];

                    for (int k = 0; argv[0][k]; k++)
                        full_path[j++] = argv[0][k];

                    full_path[j] = 0;

                    do_exec(full_path);
                }
            }

            write(1, "sh: command not found\n", 22);
            exit(1);
        }
        else
            wait4(-1, 0, 0, 0);
    }
}