#include <user.h>

int unlink(const char* path)
{
    int ret;

    asm volatile (
        "mov $87, %%eax\n"
        "syscall"
        : "=a"(ret)
        : "D"(path)
        : "rcx", "r11", "memory"
        );

    return ret;
}

struct Dirent
{
    unsigned long ino;
    unsigned int type;
    char name[32];
};

#define IT_DIR 0x4000
#define IT_REG 0x8000

#define MAX_LINE_SIZE 80
#define MAX_LINES 1000

struct Line
{
    char data[MAX_LINE_SIZE + 1];
    int size = 0;

    bool push_back(char c)
    {
        if (size == MAX_LINE_SIZE)
            return false;

        data[size++] = c;
        data[size] = 0;

        return true;
    }

    bool insert(int pos, char c)
    {
        if (size == MAX_LINE_SIZE)
            return false;

        if (pos == size)
            return push_back(c);

        memmove(data + pos + 1, data + pos, size - pos);
        data[pos] = c;
        size++;

        data[size] = 0;

        return true;
    }

    void erase(int pos)
    {
        if (pos < 0 || pos >= size)
            return;

        memmove(data + pos, data + pos + 1, size - pos - 1);
        size--;

        data[size] = 0;
    }
};

struct Editor;

void draw(Editor* ed);
void set_raw();
void move_cursor(int line, int col);
void set_cooked();
void clear_screen();

struct Editor
{
    Line cwd;
    char real_cwd[256];
    int cursor = 0;
    int sel_line = 2;
    Dirent sel_dent;
    bool has_sel = false;
    bool on_new = false;
    Line new_name;
    int new_cursor = 0;

    void insert(char c)
    {
        if (on_new)
        {
            if (new_name.insert(new_cursor, c))
                new_cursor++;
        }
        else
        {
            if (cwd.insert(cursor, c))
                cursor++;
        }
    }

    void backspace_one()
    {
        if (cursor > 0)
        {
            cwd.erase(cursor - 1);
            cursor--;
        }
    }

    void backspace()
    {
        if (on_new)
        {
            if (new_cursor > 0)
            {
                new_name.erase(new_cursor - 1);
                new_cursor--;
            }
        }
        else
        {
            if (cwd.data[cwd.size - 1] == '/' && cwd.size > 1)
                backspace_one();

            while (cwd.size > 0 && cwd.data[cwd.size - 1] != '/')
                backspace_one();
        }
    }

    void move_right()
    {
        if (on_new)
        {
            if (new_cursor < new_name.size)
                new_cursor++;
        }
        else
        {
            if (cursor < cwd.size)
                cursor++;
        }
    }

    void move_left()
    {
        if (on_new)
        {
            if (new_cursor > 0)
                new_cursor--;
        }
        else
        {
            if (cursor > 0)
                cursor--;
        }
    }

    void move_home()
    {
        if (on_new)
            new_cursor = 0;
        else
            cursor = 0;
    }

    void move_end()
    {
        if (on_new)
            new_cursor = new_name.size;
        else
            cursor = cwd.size;
    }

    void move_up()
    {
        if (sel_line > 0)
            sel_line--;
    }

    void move_down()
    {
        if (on_new)
            return;

        sel_line++;
    }

    void _delete()
    {
        if (!has_sel)
            return;

        if ((sel_dent.type & IT_DIR) == 0)
            unlink(sel_dent.name);
        else
            rmdir(sel_dent.name);
    }

    void enter_ed()
    {
        int pid = fork();

        if (pid == 0)
        {
            char* argv[3];
            argv[0] = (char*)"/mnt/bin/ed";
            argv[1] = (char*)sel_dent.name;
            argv[2] = nullptr;

            execve(argv[0], argv, nullptr);
            exit(1);
        }
        else
        {
            wait4(-1, 0, 0, 0);
            set_raw();
        }
    }

    void enter_dir()
    {
        int len = strlen(sel_dent.name);

        cursor = cwd.size;

        if (strcmp(sel_dent.name, ".") == 0)
            return;

        if (strcmp(sel_dent.name, "..") == 0)
        {
            backspace();
        }
        else
        {
            if (cwd.data[cwd.size - 1] != '/')
                insert('/');

            for (int i = 0; i < len; i++)
                insert(sel_dent.name[i]);

            insert('/');
        }
    }

    void enter()
    {
        if (!has_sel)
        {
            if (on_new)
            {
                if (new_name.size > 0)
                {
                    if (new_name.data[new_name.size - 1] == '/')
                    {
                        new_name.data[new_name.size - 1] = 0;

                        int err = mkdir(new_name.data, 0);

                        new_name.data[new_name.size - 1] = '/';

                        if (!err)
                        {
                            new_name.size = 0;
                            new_name.data[0] = 0;
                            new_cursor = 0;
                        }
                    }
                    else
                    {
                        int fd = open(new_name.data, O_EXCL | O_CREAT, 0);

                        if (fd > 0)
                        {
                            close(fd);
                            new_name.size = 0;
                            new_name.data[0] = 0;
                            new_cursor = 0;
                        }
                    }
                }
            }

            return;
        }

        if ((sel_dent.type & IT_DIR) == IT_DIR)
            enter_dir();
        else if ((sel_dent.type & IT_REG) == IT_REG)
            enter_ed();
    }

    void switch_to_shell()
    {
        int pid = fork();

        if (pid == 0)
        {
            char* argv[2];
            argv[0] = (char*)"/mnt/bin/sh";
            argv[1] = nullptr;

            clear_screen();
            set_cooked();

            execve(argv[0], argv, nullptr);
            exit(1);
        }
        else
        {
            wait4(-1, 0, 0, 0);
            set_raw();
        }
    }
};

enum Key
{
    TAB = '\t',
    ENTER = '\n',
    ESC = '\e',
    BACKSPACE = 127,
    DELETE = 1000,

    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    HOME,
    END,

    SHIFT_ARROW_LEFT,
    SHIFT_ARROW_RIGHT,
    SHIFT_ARROW_UP,
    SHIFT_ARROW_DOWN,
    SHIFT_HOME,
    SHIFT_END,

    CTRL_Q,
    CTRL_S,
    CTRL_A,
    CTRL_X,
    CTRL_L
};

bool getchar(char* ch)
{
    if (read(0, ch, 1) == 1)
        return true;

    return false;
}

#define CTRL(x) ((x) - 'A' + 1)

int get_key()
{
    char c;

    if (!getchar(&c))
        return 0;

    if (c == ESC)
    {
        char seq[5];

        if (!getchar(seq))
            return ESC;

        if (seq[0] != '[')
            return seq[0];

        if (!getchar(seq + 1))
            return ESC;

        if (isdigit(seq[1]))
        {
            if (!getchar(seq + 2))
                return ESC;

            if (seq[2] == '~' && seq[1] == '3')
                return DELETE;

            if (seq[1] == '1')
            {
                if (!getchar(seq + 3) || !getchar(seq + 4))
                    return ESC;

                if (seq[2] == ';' && seq[3] == '2')
                {
                    switch (seq[4])
                    {
                    case 'A': return SHIFT_ARROW_UP;
                    case 'B': return SHIFT_ARROW_DOWN;
                    case 'C': return SHIFT_ARROW_RIGHT;
                    case 'D': return SHIFT_ARROW_LEFT;
                    case 'H': return SHIFT_HOME;
                    case 'F': return SHIFT_END;
                    }
                }
            }
        }
        else
        {
            switch (seq[1])
            {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            case 'H': return HOME;
            case 'F': return END;
            }
        }

        return seq[1];
    }
    else if (c == CTRL('A'))    return CTRL_A;
    else if (c == CTRL('S'))    return CTRL_S;
    else if (c == CTRL('Q'))    return CTRL_Q;
    else if (c == CTRL('X'))    return CTRL_X;
    else if (c == CTRL('L'))    return CTRL_L;
    else                return c;
}

bool quit = false;

void process_keys(Editor* ed)
{
    int key = get_key();

    if (!key)
        return;

    if (isprint(key))
    {
        if (key == '`')
            ed->switch_to_shell();
        else
            ed->insert(key);
    }
    else
    {
        switch (key)
        {
        case CTRL_Q:            quit = true;                    break;
        case BACKSPACE:         ed->backspace();                break;
        case ARROW_LEFT:        ed->move_left();                break;
        case ARROW_RIGHT:       ed->move_right();               break;
        case HOME:              ed->move_home();                break;
        case END:               ed->move_end();                 break;
        case ARROW_UP:          ed->move_up();                  break;
        case ARROW_DOWN:        ed->move_down();                break;
        case '\n':              ed->enter();                    break;
        case CTRL_X:
        case DELETE:            ed->_delete();                  break;
        }
    }

    draw(ed);
}

inline void hide_cursor()
{
    printf("\e[25l");
}

inline void show_cursor()
{
    printf("\e[25h");
}

inline void move_cursor(int line, int col)
{
    printf("\e[%d;%dH", line + 1, col + 1);
}

inline void clear_screen()
{
    // printf("\e[2J\e[H");
    ioctl(1, FBTERM_CLEAR, nullptr);
}

inline void clear_eol()
{
    printf("\e[m\e[K");
}

int get_height()
{
    int height;
    ioctl(1, FBTERM_GET_HEIGHT, &height);

    return height;
}

int get_width()
{
    int width;
    ioctl(1, FBTERM_GET_WIDTH, &width);

    return width;
}

void set_raw()
{
    ioctl(0, FBTERM_ECHO_OFF, nullptr);
    ioctl(0, FBTERM_LB_OFF, nullptr);

    hide_cursor();

    auto_flush = false;
}

void set_cooked()
{
    ioctl(0, FBTERM_ECHO_ON, nullptr);
    ioctl(0, FBTERM_LB_ON, nullptr);

    show_cursor();
    flush();
}

bool is_elf(int fd)
{
    char hdr[4];
    int ret = read(fd, hdr, 4);
    seek(fd, 0, SEEK_SET);

    return ret == 4 && hdr[0] == 0x7f && hdr[1] == 'E' && hdr[2] == 'L' && hdr[3] == 'F';
}

void draw(Editor* ed)
{
    int height = get_height();
    int width = get_width();

    int line = 0;

    move_cursor(line, 0);

    for (int j = 0; j <= ed->cwd.size; j++)
    {
        char ch = j < ed->cwd.size ? ed->cwd.data[j] : ' ';

        // if (j == ed->cursor)
        // {
        //     printf("\e[107;30m%c\e[m", ch);
        //     continue;
        // }

        printf(ch == '/' ? "\e[91m" : "\e[94m");
        printf("%c", ch);
    }

    clear_eol();
    line++;

    ed->has_sel = false;
    ed->on_new = false;

    int fd = ed->cwd.size == 0 ? -1 : open(ed->cwd.data, O_RDONLY | O_DIRECTORY, 0);

    if (fd < 0)
    {
        move_cursor(line, 0);
        printf("dir not found");
        clear_eol();
        line++;
    }
    else
    {
        if (strcmp(ed->cwd.data, ed->real_cwd) != 0)
        {
            ed->sel_line = 2;
            strcpy(ed->real_cwd, ed->cwd.data);
            chdir(ed->cwd.data);
        }

        Dirent dents[64];
        long bytes_read = 0;
        int ls_line = 0;

        while ((bytes_read = getdents(fd, dents, sizeof(dents))) > 0)
        {
            int count = bytes_read / sizeof(Dirent);

            for (Dirent* dent = dents; dent < dents + count; dent++)
            {
                bool at_sel = ls_line == ed->sel_line;

                move_cursor(line, 0);

                printf("    ");

                if (at_sel)
                    printf("\e[7m");

                printf((dent->type & IT_DIR) == IT_DIR ? "\e[94m%s\e[m " : "%s", dent->name);

                if (at_sel)
                    printf("\e[27m");

                clear_eol();

                if (at_sel)
                {
                    ed->sel_dent = *dent;
                    ed->has_sel = true;
                }

                line++;
                ls_line++;
            }
        }

        close(fd);

        if (ed->sel_line >= ls_line)
        {
            ed->on_new = true;
            move_cursor(line, 0);
            printf("  \e[90m+\e[m ");

            bool is_dir = ed->new_name.size > 0 && ed->new_name.data[ed->new_name.size - 1] == '/';

            for (int i = 0; i <= ed->new_name.size; i++)
            {
                char ch = i < ed->new_name.size ? ed->new_name.data[i] : ' ';

                if (i == ed->new_cursor)
                    printf("\e[107;30m%c\e[m", ch);
                else
                {
                    if (is_dir && ch != '/')
                        printf("\e[94m%c\e[m", ch);
                    else
                        printf("%c", ch);
                }
            }

            clear_eol();
            line++;
        }
    }

    for (int i = line; i < height; i++)
    {
        move_cursor(i, 0);
        clear_eol();
    }

    int split = 60;
    line = 0;

    if (ed->has_sel && (ed->sel_dent.type & IT_REG) == IT_REG)
    {
        int fd = open(ed->sel_dent.name, O_RDONLY, 0);

        move_cursor(line, split);
        printf("\e[90m");

        if (is_elf(fd))
        {
            printf("ELF file");
            line++;
        }
        else
        {
            int left = width - split;
            char buf[512];

            while (true)
            {
                int bytes_read = read(fd, buf, sizeof(buf));

                if (bytes_read <= 0)
                {
                    line++;
                    break;
                }

                for (int i = 0; i < bytes_read; i++)
                {
                    if (buf[i] == '\n')
                    {
                        line++;

                        if (line >= height)
                            break;

                        move_cursor(line, split);
                        left = width - split;
                    }
                    else
                    {
                        if (left > 0)
                        {
                            printf("%c", buf[i]);
                            left--;
                        }
                    }
                }

                if (line >= height)
                    break;
            }
        }

        close(fd);
    }
    else if (ed->has_sel && (ed->sel_dent.type & IT_DIR) == IT_DIR)
    {
        int fd = open(ed->sel_dent.name, O_RDONLY | O_DIRECTORY, 0);

        Dirent dents[64];
        long bytes_read = 0;

        while ((bytes_read = getdents(fd, dents, sizeof(dents))) > 0)
        {
            int count = bytes_read / sizeof(Dirent);

            for (Dirent* dent = dents; dent < dents + count; dent++)
            {
                move_cursor(line, split);
                printf("\e[90m");

                printf("%s", dent->name);

                clear_eol();
                line++;
            }
        }

        close(fd);
    }

    // for (int i = line; i < height; i++)
    // {
    //     move_cursor(i, 0);
    //     clear_eol();
    // }

    flush();
}

int main(int argc, char** argv)
{
    Editor ed;

    getcwd(ed.cwd.data, MAX_LINE_SIZE);
    ed.cwd.size = strlen(ed.cwd.data);
    ed.cursor = ed.cwd.size;
    strcpy(ed.real_cwd, ed.cwd.data);

    set_raw();

    draw(&ed);

    while (!quit)
        process_keys(&ed);

    clear_screen();

    set_cooked();

    return 0;
}