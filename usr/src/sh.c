#include <sys.h>

usize strlen(const char* str)
{
    usize len = 0;

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

char* strcpy(char* dest, const char* src)
{
    char* ret = dest;

    while (*src)
    {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = 0;

    return ret;
}

char* strcat(char* dest, const char* src)
{
    char* ret = dest;

    while (*dest)
        dest++;

    while (*src)
    {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = 0;

    return ret;
}

char* strchr(char* str, char ch)
{
    char* ret = str;

    while (*str)
    {
        if (*str == ch)
            return str;

        str++;
    }

    return 0;
}

void fprintln(int fd, const char* str)
{
    write(fd, str, strlen(str));
    write(fd, "\n", 1);
}

void println(const char* str)
{
    fprintln(STDOUT_FILENO, str);
}

void eprintln(const char* str)
{
    fprintln(STDERR_FILENO, str);
}

int isatty(int fd)
{
    int dummy;
    return ioctl(fd, FBTERM_GET_WIDTH, &dummy) == 0;
}

#define BUF_SIZE    1024
#define ARG_MAX     16
#define CMD_MAX     16
#define CWD_SIZE    256

char readline_buf[BUF_SIZE];
char* line;

struct command
{
    int argc;
    char* argv[ARG_MAX];
};

struct command cmds[CMD_MAX];
int cmd_count;
char cwd_buf[CWD_SIZE];

const char* paths[] =
{
    "/mnt/bin/",
    "/bin/"
};

char exec_path[256];
int fg_group = -1;

char* trim(char* str)
{
    char* end = str + strlen(str) - 1;

    while (end >= str && *end == ' ')
        end--;

    end[1] = 0;

    while (*str == ' ')
        str++;

    return str;
}

void build_args(char* line)
{
    cmd_count = 0;
    cmds[0].argc = 0;
    char* ptr = trim(line);

    // empty line
    if (*ptr == 0)
        return;

    while (1)
    {
        while (*ptr == ' ')
            ptr++;

        if (*ptr == 0)
            break;

        char* start;

        if (*ptr == '\'')
        {
            ptr++;
            start = ptr;

            while (*ptr && *ptr != '\'')
                ptr++;

            if (*ptr == '\'')
                *ptr++ = 0;
        }
        else
        {
            start = ptr;

            while (*ptr && *ptr != ' ')
                ptr++;

            if (*ptr == ' ')
                *ptr++ = 0;
        }

        cmds[cmd_count].argv[cmds[cmd_count].argc++] = start;

        if (strcmp(start, "|") == 0)
        {
            cmds[cmd_count].argc--;
            cmds[cmd_count].argv[cmds[cmd_count].argc] = 0;
            cmd_count++;
            cmds[cmd_count].argc = 0;
        }
    }

    cmds[cmd_count].argv[cmds[cmd_count].argc] = 0;
    cmd_count++;
}

void print_prompt()
{
    // we should probably only build the prompt once
    // and then just update it when we change directories

    getcwd(cwd_buf, CWD_SIZE);

    for (int i = 0; cwd_buf[i]; i++)
    {
        if (cwd_buf[i] == '/')
            write(1, "\e[91m/\e[94m", 11);
        else
            write(1, cwd_buf + i, 1);
    }

    write(1, "\e[91m>\e[m ", 10);
}

enum readline_result
{
    READLINE_OK = 0,
    READLINE_EMPTY,
    READLINE_EOF,
    READLINE_ERROR,
};

int readline()
{
    // we need to reset the terminal state
    // because some programs may mess with it

    ioctl(STDIN_FILENO, FBTERM_LB_ON, 0);
    ioctl(STDIN_FILENO, FBTERM_ECHO_ON, 0);
    write(STDIN_FILENO, "\e[25h", 5);

    i64 bytes = read(0, readline_buf, BUF_SIZE - 1);

    if (bytes < 0)
        return READLINE_ERROR;

    if (bytes == 0)
        return READLINE_EOF;

    if (readline_buf[bytes - 1] != '\n')
        write(1, "\n", 1);
    else
    {
        bytes--; // remove the newline

        if (bytes == 0)
            return READLINE_EMPTY;
    }

    // null terminate the buffer
    readline_buf[bytes] = 0;

    char* end = readline_buf + bytes - 1;

    // right trim
    while (end >= readline_buf && *end == ' ')
        end--;

    if (end < readline_buf)
        return READLINE_EMPTY;

    end[1] = 0;

    // now line points to the first character of the line
    line = readline_buf;

    while (*line == ' ')
        line++;

    return READLINE_OK;
}

int try_exec_internal(struct command* cmd)
{
    if (strcmp(cmd->argv[0], "exit") == 0)
        exit(0);

    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        if (cmd->argc > 1)
        {
            int ret = chdir(cmd->argv[1]);

            if (ret != 0)
                println("cd: no such directory");
        }
        else
            chdir("/");

        return 1;
    }

    return 0;
}

int is_executable(const char* path)
{
    struct stat st;

    if (stat(path, &st) < 0 || (st.st_mode & IT_MASK) != IT_REG || st.st_mode & IP_X == 0)
        return 0;

    return 1;
}

char* find_executable(char* name)
{
    if (strchr(name, '/') != 0)
        return is_executable(name) ? name : 0;

    for (int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
    {
        strcpy(exec_path, paths[i]);
        strcat(exec_path, name);

        if (is_executable(exec_path))
            return exec_path;
    }

    return 0;
}

void redirect(int fd, const char* filename, u32 flags, u32 mode)
{
    int new_fd = open(filename, flags, mode);
    if (new_fd < 0)
    {
        eprintln("sh: open failed");
        return;
    }

    dup2(new_fd, fd);
    close(new_fd);
}

int exec_line(char* line)
{
    build_args(line);

    if (cmd_count == 0)
        return 0;

    int pipe_count = cmd_count - 1;
    int pipes[pipe_count][2];

    if (!pipe_count && try_exec_internal(cmds))
        return 0;

    for (int i = 0; i < pipe_count; i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            eprintln("sh: pipe failed");

            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return 1;
        }
    }

    for (int i = 0; i < cmd_count; i++)
    {
        int pid = fork();

        if (pid != 0)
            continue;

        if (fg_group > 0)
            setgroup(fg_group);

        if (try_exec_internal(cmds + i))
            exit(0);

        char* exec_path = find_executable(cmds[i].argv[0]);

        if (!exec_path)
        {
            eprintln("sh: command not found");
            exit(1);
        }

        if (i > 0)
            dup2(pipes[i - 1][0], 0);

        if (i < pipe_count)
            dup2(pipes[i][1], 1);

        for (int i = 0; i < pipe_count; i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        int argc = cmds[i].argc;

        if (argc > 2)
        {
            char* filename = cmds[i].argv[argc - 1];
            char* redir = cmds[i].argv[argc - 2];

            if (strcmp(redir, ">") == 0)
            {
                cmds[i].argv[argc - 2] = 0;

                if (i == cmd_count - 1)
                    redirect(1, filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            else if (strcmp(redir, "<") == 0)
            {
                cmds[i].argv[argc - 2] = 0;

                if (i == 0)
                    redirect(0, filename, O_RDONLY, 0);
            }
        }

        execve(exec_path, cmds[i].argv, 0);

        eprintln("sh: execve failed");
        exit(1);
    }

    for (int i = 0; i < pipe_count; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    while (wait(-1, 0, 0) > 0);

    // should return the exit status of the last command
    return 0;
}

int repl()
{
    // set the foreground group to the current group + 1
    fg_group = setgroup(-1) + 1;
    ioctl(1, FBTERM_SET_FG_GROUP, &fg_group);

    while (1)
    {
        print_prompt();

        int res = readline();

        if (res == READLINE_ERROR || res == READLINE_EMPTY)
            continue;

        if (res == READLINE_EOF)
        {
            println("exit");
            break;
        }

        exec_line(line);

        ioctl(1, FBTERM_SET_FG_GROUP, &fg_group);
    }

    return 0;
}

int exec_file(int fd)
{
    char buf[BUF_SIZE];
    char line[BUF_SIZE];
    int line_len = 0;
    isize bytes;

    while ((bytes = read(fd, buf, BUF_SIZE)) > 0)
    {
        for (isize i = 0; i < bytes; ++i)
        {
            if (buf[i] == '\n')
            {
                line[line_len] = 0;

                if (line_len > 0)
                    exec_line(line);

                line_len = 0;
            }
            else if (line_len < BUF_SIZE - 1)
            {
                line[line_len++] = buf[i];
            }
            else
            {
                eprintln("sh: line too long");
                return 1;
            }
        }
    }

    if (line_len > 0)
    {
        line[line_len] = 0;
        exec_line(line);
    }

    if (bytes < 0)
    {
        eprintln("sh: read error");
        return 1;
    }

    return 0;
}

void print_usage(int fd)
{
    fprintln(fd,
        "usage: sh [file]\n"
        "       sh -c <command>\n"
        "       sh [-h | --help]\n"
        "\n"
        "options:\n"
        "  -c <command>   Execute the specified command string.\n"
        "  -h, --help     Show this help message and exit.\n"
        "\n"
        "If file is provided, commands are read from the file.\n"
        "If no file is provided, input is read from stdin.\n"
        "If stdin is a TTY, the shell runs in interactive mode.");
}

int main(int argc, char** argv)
{
    int fd;

    // open stdin, stdout and stderr if not already open
    while ((fd = open("/dev/fbterm", O_RDWR, 0)) >= 0)
    {
        if (fd > 2)
        {
            close(fd);
            break;
        }
    }

    // argc should be always > 0, but just in case
    if (argc < 1)
    {
        eprintln("sh: invalid argument count");
        goto err;
    }

    if (argc == 1)
        return isatty(STDIN_FILENO) ? repl() : exec_file(STDIN_FILENO);

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_usage(STDOUT_FILENO);
        return 0;
    }

    if (strcmp(argv[1], "-c") == 0)
    {
        if (argc >= 3)
            return exec_line(argv[2]);

        eprintln("sh: -c requires an argument");
        goto err;
    }

    if (argv[1][0] == '-')
    {
        eprintln("sh: unknown option");
        goto err;
    }

    fd = open(argv[1], O_RDONLY, 0);

    if (fd < 0)
    {
        eprintln("sh: open failed");
        return 1;
    }

    int ret = exec_file(fd);
    close(fd);
    return ret;

err:
    print_usage(STDERR_FILENO);
    return 1;
}
