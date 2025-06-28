#include <user.h>

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

enum Redraw
{
    ALL,
    JUST_CWD,
    JUST_NEWNAME,
    NOTHING
};

Redraw redraw;

struct Entry
{
    char display_name[40];
    char name[32];
    struct stat st;
};

struct Editor;

void draw(Editor* ed);
void set_raw();
void move_cursor(int line, int col);
void set_cooked();
void clear_screen();

#define MAX_ENTRIES 100

struct Editor
{
    char real_cwd[256];
    Line cwd;
    int cursor = 0;
    int sel_line = 1;
    Line new_name;
    int new_cursor = 0;

    struct Entry entries[MAX_ENTRIES];
    int entry_count = 0;

    void get_entries()
    {
        entry_count = 0;

        int fd = cwd.size == 0 ? -1 : open(cwd.data, O_RDONLY | O_DIRECTORY, 0);

        if (fd < 0)
        {
            sel_line = -1;
            return;
        }

        if (strcmp(cwd.data, real_cwd) != 0)
        {
            if (sel_line >= 0)
                sel_line = 1;

            strcpy(real_cwd, cwd.data);
            chdir(cwd.data);
        }

        Dirent dents[64];
        long bytes_read = 0;

        while ((bytes_read = getdents(fd, dents, sizeof(dents))) > 0)
        {
            int count = bytes_read / sizeof(Dirent);

            for (Dirent* dent = dents; dent < dents + count; dent++)
            {
                if (dent->name[0] == '.' && dent->name[1] == 0)
                    continue;

                Entry* ent = entries + entry_count;

                if (stat(dent->name, &ent->st) < 0)
                    continue;

                strcpy(ent->name, dent->name);

                int type = ent->st.st_mode & IT_MASK;
                int perm = ent->st.st_mode & IP_MASK;

                if (type == IT_DIR)
                    strcpy(ent->display_name, "\e[94m");
                else if (type == IT_CDEV || type == IT_BDEV)
                    strcpy(ent->display_name, "\e[93m");
                else if (perm & IP_X)
                    strcpy(ent->display_name, "\e[92m");
                else
                    strcpy(ent->display_name, "\e[39m");

                strcat(ent->display_name, dent->name);

                entry_count++;
            }
        }

        close(fd);
    }

    void insert(char c)
    {
        if (entry_count > 0 && sel_line == entry_count)
        {
            if (new_name.insert(new_cursor, c))
                new_cursor++;

            redraw = JUST_NEWNAME;
        }
        else if (sel_line == -1)
        {
            if (cwd.insert(cursor, c))
            {
                cursor++;
                get_entries();
            }
        }
        else
            redraw = NOTHING;
    }

    void backspace_one()
    {
        if (cursor > 1)
        {
            cwd.erase(cursor - 1);
            cursor--;
        }
    }

    void backspace_dir()
    {
        cursor = cwd.size;

        if (cwd.data[cwd.size - 1] == '/' && cwd.size > 1)
            backspace_one();

        while (cwd.size > 0 && cwd.data[cwd.size - 1] != '/')
            backspace_one();
    }

    void backspace()
    {
        if (entry_count > 0 && sel_line == entry_count)
        {
            if (new_cursor > 0)
            {
                new_name.erase(new_cursor - 1);
                new_cursor--;
            }

            redraw = JUST_NEWNAME;
        }
        else
        {
            if (sel_line == -1 && cursor != cwd.size)
                backspace_one();
            else
                backspace_dir();

            get_entries();
        }
    }

    void move_right()
    {
        if (entry_count > 0 && sel_line == entry_count)
        {
            if (new_cursor < new_name.size)
                new_cursor++;

            redraw = JUST_NEWNAME;
        }
        else if (sel_line == -1)
        {
            if (cursor < cwd.size)
                cursor++;

            redraw = JUST_CWD;
        }
        else
            redraw = NOTHING;
    }

    void move_left()
    {
        if (entry_count > 0 && sel_line == entry_count)
        {
            if (new_cursor > 0)
                new_cursor--;

            redraw = JUST_NEWNAME;
        }
        else if (sel_line == -1)
        {
            if (cursor > 0)
                cursor--;

            redraw = JUST_CWD;
        }
        else
            redraw = NOTHING;
    }

    void move_home()
    {
        if (sel_line == -1)
        {
            cursor = 0;
            redraw = JUST_CWD;
        }
        else if (entry_count > 0)
        {
            if (sel_line >= 0 && sel_line < entry_count)
                sel_line = -1;
            else if (sel_line == entry_count)
            {
                new_cursor = 0;
                redraw = JUST_NEWNAME;
            }
        }
        else
            redraw = NOTHING;
    }

    void move_end()
    {
        if (sel_line == -1)
        {
            cursor = cwd.size;
            redraw = JUST_CWD;
        }
        else if (entry_count > 0)
        {
            if (sel_line >= 0 && sel_line < entry_count)
                sel_line = entry_count;
            else if (sel_line == entry_count)
            {
                new_cursor = new_name.size;
                redraw = JUST_NEWNAME;
            }
        }
        else
            redraw = NOTHING;
    }

    void move_up()
    {
        if (sel_line >= 0)
            sel_line--;
        else
            redraw = NOTHING;
    }

    void move_down()
    {
        if (entry_count > 0 && sel_line < entry_count)
            sel_line++;
        else
            redraw = NOTHING;
    }

    void _delete()
    {
        if (entry_count == 0 || sel_line < 0 || sel_line >= entry_count)
            return;

        int err;

        if ((entries[sel_line].st.st_mode & IT_MASK) == IT_DIR)
            err = rmdir(entries[sel_line].name);
        else
            err = unlink(entries[sel_line].name);

        if (!err)
            get_entries();
        else
            redraw = NOTHING;
    }

    void enter_ed()
    {
        int pid = fork();

        if (pid == 0)
        {
            char* argv[3];
            argv[0] = (char*)"/mnt/bin/ed";
            argv[1] = (char*)entries[sel_line].name;
            argv[2] = nullptr;

            execve(argv[0], argv, nullptr);
            exit(1);
        }
        else
        {
            wait(-1, 0, 0);
            set_raw();
        }
    }

    void enter_dir()
    {
        if (strcmp(entries[sel_line].name, ".") == 0)
        {
            redraw = NOTHING;
            return;
        }

        if (strcmp(entries[sel_line].name, "..") == 0)
        {
            backspace();
            return;
        }

        int len = strlen(entries[sel_line].name);
        int req_size = len + 1;

        if (cwd.data[cwd.size - 1] != '/')
            req_size++;

        if (cwd.size + req_size > MAX_LINE_SIZE)
        {
            redraw = NOTHING;
            return;
        }

        if (cwd.data[cwd.size - 1] != '/')
            cwd.insert(cwd.size, '/');

        strcat(cwd.data, entries[sel_line].name);
        cwd.size += len;
        cwd.insert(cwd.size, '/');

        cursor = cwd.size;

        get_entries();
    }

    void enter()
    {
        if (entry_count == 0)
        {
            redraw = NOTHING;
            return;
        }

        if (sel_line >= 0 && sel_line < entry_count)
        {
            int type = entries[sel_line].st.st_mode & IT_MASK;

            if (type == IT_DIR)
                enter_dir();
            else if (type == IT_REG)
                enter_ed();

            return;
        }

        if (sel_line != entry_count || new_name.size == 0)
        {
            redraw = NOTHING;
            return;
        }

        if (new_name.data[new_name.size - 1] == '/')
        {
            new_name.data[new_name.size - 1] = 0;

            int err = mkdir(new_name.data);

            new_name.data[new_name.size - 1] = '/';

            if (!err)
            {
                new_name.size = 0;
                new_name.data[0] = 0;
                new_cursor = 0;
                get_entries();
            }
            else
                redraw = NOTHING;
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
                get_entries();
            }
            else
                redraw = NOTHING;
        }
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
            wait(-1, 0, 0);
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

    redraw = ALL;

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
        case CTRL_Q:            quit = true; redraw = NOTHING;  break;
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

void draw_cwd(Editor* ed)
{
    move_cursor(0, 0);

    for (int j = 0; j <= ed->cwd.size; j++)
    {
        char ch = j < ed->cwd.size ? ed->cwd.data[j] : ' ';

        if (ed->sel_line == -1 && j == ed->cursor)
            printf("\e[107;30m%c\e[m", ch);
        else
            printf("\e[9%cm%c", ch == '/' ? '1' : '4', ch);
    }

    clear_eol();
}

void draw_newname(Editor* ed)
{
    move_cursor(1 + ed->entry_count, 0);
    printf("  \e[37m+\e[m ");

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
}

void draw(Editor* ed)
{
    if (redraw == NOTHING)
        return;

    if (redraw == JUST_CWD)
    {
        draw_cwd(ed);
        flush();
        return;
    }
    else if (redraw == JUST_NEWNAME)
    {
        draw_newname(ed);
        flush();
        return;
    }

    int height = get_height();
    int width = get_width();
    int split = width / 3;
    int line = 0;

    draw_cwd(ed);
    line++;

    if (ed->entry_count > 0)
    {
        for (int i = 0; i < ed->entry_count; i++)
        {
            move_cursor(line, 0);
            printf("    %s%s", i == ed->sel_line ? "\e[7m" : "", ed->entries[i].display_name);
            clear_eol();
            line++;
        }
    }

    if (ed->sel_line == ed->entry_count)
    {
        draw_newname(ed);
        line++;
    }

    for (int i = line; i < height; i++)
    {
        move_cursor(i, 0);
        clear_eol();
    }

    if (ed->entry_count == 0 || ed->sel_line == ed->entry_count)
    {
        flush();
        return;
    }

    line = 0;
    Entry* ent = ed->entries + ed->sel_line;
    int type = ent->st.st_mode & IT_MASK;

    printf("\e[37m");

    if (type == IT_REG)
    {
        int fd = open(ent->name, O_RDONLY, 0);

        move_cursor(line, split);

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
    else if (type == IT_DIR)
    {
        int fd = open(ent->name, O_RDONLY | O_DIRECTORY, 0);

        Dirent dents[64];
        long bytes_read = 0;

        while ((bytes_read = getdents(fd, dents, sizeof(dents))) > 0)
        {
            int count = bytes_read / sizeof(Dirent);

            for (Dirent* dent = dents; dent < dents + count; dent++)
            {
                if (dent->name[0] == '.' && dent->name[1] == 0)
                    continue;

                move_cursor(line, split);
                printf("\e[37m%s", dent->name);
                line++;
            }
        }

        close(fd);
    }
    else if (type == IT_CDEV || type == IT_BDEV)
    {
        move_cursor(line, split);
        printf("Device file");
        line++;
    }

    flush();
}

int main(int argc, char** argv)
{
    Editor ed;

    getcwd(ed.cwd.data, MAX_LINE_SIZE);
    ed.cwd.size = strlen(ed.cwd.data);
    ed.cursor = ed.cwd.size;
    strcpy(ed.real_cwd, ed.cwd.data);
    ed.get_entries();

    set_raw();

    draw(&ed);

    while (!quit)
        process_keys(&ed);

    clear_screen();

    set_cooked();

    return 0;
}
