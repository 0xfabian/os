#include <user.h>

#define MAX_LINE_SIZE 120
#define MAX_LINES 1000

bool quit = false;
int term_fd;
int view_start_line = 0;

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
    ioctl(term_fd, FBTERM_CLEAR, nullptr);
}

int get_width()
{
    int width;
    ioctl(term_fd, FBTERM_GET_WIDTH, &width);

    return width;
}

int get_height()
{
    int height;
    ioctl(term_fd, FBTERM_GET_HEIGHT, &height);

    return height;
}

void set_raw()
{
    ioctl(term_fd, FBTERM_ECHO_OFF, nullptr);
    ioctl(term_fd, FBTERM_LB_OFF, nullptr);

    hide_cursor();

    auto_flush = false;
}

void set_cooked()
{
    ioctl(term_fd, FBTERM_ECHO_ON, nullptr);
    ioctl(term_fd, FBTERM_LB_ON, nullptr);

    show_cursor();
    flush();
}

struct Line
{
    char data[MAX_LINE_SIZE + 1];
    int size;

    bool push_back(char c)
    {
        if (size == MAX_LINE_SIZE)
            return false;

        data[size++] = c;
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

        return true;
    }

    void erase(int pos)
    {
        if (pos < 0 || pos >= size)
            return;

        memmove(data + pos, data + pos + 1, size - pos - 1);
        size--;
    }

    void from(Line* other)
    {
        memcpy(data, other->data, other->size);
        size = other->size;
    }
};

struct Text
{
    Line lines[MAX_LINES];
    int size;

    bool push_back_empty()
    {
        if (size == MAX_LINES)
            return false;

        size++;
        memset(lines[size - 1].data, 0, MAX_LINE_SIZE);
        lines[size - 1].size = 0;

        return true;
    }

    bool insert_empty(int pos)
    {
        if (size == MAX_LINES)
            return false;

        if (pos == size)
            return push_back_empty();

        memmove(lines + pos + 1, lines + pos, (size - pos) * sizeof(Line));
        memset(lines[pos].data, 0, MAX_LINE_SIZE);
        lines[pos].size = 0;
        size++;

        return true;
    }

    void erase(int pos)
    {
        if (pos < 0 || pos >= size)
            return;

        memmove(lines + pos, lines + pos + 1, (size - pos - 1) * sizeof(Line));
        size--;
    }
};

struct Token
{
    char* start;
    int size;
    int color;
};

char color_buf[MAX_LINE_SIZE + 1];
Text bss_text;

Token next_c_token(char*&);
Token next_asm_token(char*&);
Token next_regular_token(char*&);

struct Editor
{
    int line;
    int col;
    int target_col;
    bool vertical_move;

    bool is_selection;
    int sel_line;
    int sel_col;

    char* filename;
    int fd;
    bool dirty;
    Token(*next_token)(char*&);

    Text* text;

    Editor(char* _filename);
    ~Editor();

    bool save();
    bool write_to_fd(int fd);
    bool read_from_fd(int fd);

    void insert_tab();
    void insert_newline();
    void insert_char(char c);
    void insert(char c);
    void erase();
    void backspace();

    void move_right(bool shift = false);
    void move_left(bool shift = false);
    void move_up(bool shift = false);
    void move_down(bool shift = false);
    void move_to_line_start(bool shift = false);
    void move_to_line_end(bool shift = false);

    void swap_up();
    void swap_down();

    void page_up();
    void page_down();

    void start_selection();
    void select_all();
    void select_line();
    void select_word();
    void erase_selection();
    void deselect();

    void eval_shell_command();

    void cut();
    void trim_trailing_whitespace();
    void trim_trailing_newlines();
    void append_newline_if_needed();

    usize get_size();
};

Editor::Editor(char* _filename)
{
    filename = _filename;

    line = 0;
    col = 0;
    target_col = 0;
    vertical_move = false;

    is_selection = false;
    sel_line = 0;
    sel_col = 0;

    text = &bss_text;
    text->push_back_empty();

    next_token = next_regular_token;

    if (filename)
    {
        char* ext = strrchr(filename, '.');

        if (ext)
        {
            if (strcmp(ext, ".c") == 0 || strcmp(ext, ".cpp") == 0 || strcmp(ext, ".h") == 0)
                next_token = next_c_token;
            else if (strcmp(ext, ".s") == 0 || strcmp(ext, ".S") == 0 || strcmp(ext, ".asm") == 0)
                next_token = next_asm_token;
        }

        fd = open(filename, O_RDONLY, 0);

        if (fd >= 0)
        {
            read_from_fd(fd);
            dirty = false;
            return;
        }
    }

    fd = -1;
    dirty = true;

    if (!isatty(STDIN_FILENO))
        read_from_fd(STDIN_FILENO);
}

Editor::~Editor()
{
    if (fd >= 0)
        close(fd);
}

bool Editor::save()
{
    trim_trailing_whitespace();
    trim_trailing_newlines();
    append_newline_if_needed();

    if (!filename)
        return false;

    if (fd < 0)
        // if the file was never opened, try to open so we can write
        fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0644);
    else
    {
        // if the file was opened, close it and open it
        // with write permissions and also truncate it
        close(fd);
        fd = open(filename, O_RDWR | O_TRUNC, 0);
    }

    if (fd < 0)
        return false;

    if (!write_to_fd(fd))
        return false;

    dirty = false;
    return true;
}

bool Editor::write_to_fd(int fd)
{
    for (int line = 0; line < text->size; line++)
    {
        Line* l = &text->lines[line];

        if (::write(fd, l->data, l->size) != l->size)
            return false;

        if (line != text->size - 1)
        {
            if (::write(fd, "\n", 1) != 1)
                return false;
        }
    }

    return true;
}

bool Editor::read_from_fd(int fd)
{
    char buf[4096];
    isize bytes_read;

    while ((bytes_read = read(fd, buf, 4096)) > 0)
    {
        for (int i = 0; i < bytes_read; i++)
        {
            if (buf[i] == '\n')
                text->push_back_empty();
            else
                text->lines[text->size - 1].push_back(buf[i]);
        }
    }

    return bytes_read < 0;
}

int get_prefix_whitespace(char* str, int len)
{
    int ret = 0;

    for (int i = 0; i < len; i++)
    {
        if (str[i] == ' ')
            ret++;
        else
            break;
    }

    return ret;
}

void Editor::insert_tab()
{
    int spaces = 4 - col % 4;

    for (int i = 0; i < spaces; i++)
        if (text->lines[line].insert(col, ' '))
            col++;
}

void Editor::insert_newline()
{
    if (!text->insert_empty(line + 1))
        return;

    int prefix = get_prefix_whitespace(text->lines[line].data, col);

    int prev_size = text->lines[line].size;
    text->lines[line].size = col;

    for (int i = 0; i < prefix; i++)
        text->lines[line + 1].push_back(' ');

    for (int i = col; i < prev_size; i++)
        text->lines[line + 1].push_back(text->lines[line].data[i]);

    line++;
    col = prefix;
}

void Editor::insert_char(char c)
{
    if (text->lines[line].insert(col, c))
        col++;
}

char closing = 0;
void Editor::insert(char c)
{
    dirty = true;

    erase_selection();

    if (c == '\t')
    {
        insert_tab();
    }
    else if (c == '\n')
    {
        if (col < text->lines[line].size && (col - 1) >= 0 && text->lines[line].data[col - 1] == '{' && text->lines[line].data[col] == '}')
        {
            insert_newline();
            insert_newline();
            move_up();
            insert_tab();
        }
        else
            insert_newline();
    }
    else
    {
        if (c == closing)
            move_right();
        else
            insert_char(c);

        closing = 0;

        if (c == '(')           closing = ')';
        else if (c == '[')      closing = ']';
        else if (c == '{')      closing = '}';

        if (closing)
        {
            insert_char(closing);
            move_left();
        }
    }
}

void Editor::erase()
{
    if (is_selection)
    {
        erase_selection();
        return;
    }

    if (col == text->lines[line].size)
    {
        if (line != text->size - 1)
        {
            for (int i = 0; i < text->lines[line + 1].size; i++)
                text->lines[line].push_back(text->lines[line + 1].data[i]);

            text->erase(line + 1);

            dirty = true;
        }
    }
    else
    {
        text->lines[line].erase(col);

        dirty = true;
    }
}

void Editor::backspace()
{
    if (is_selection)
    {
        erase_selection();
        return;
    }

    int whitespace = 0;

    for (int i = 0; i < col; i++)
        if (text->lines[line].data[i] == ' ')
            whitespace++;

    int spaces = 1;

    if (col > 0 && whitespace == col)
    {
        if (col % 4 == 0)
            spaces = 4;
        else
            spaces = col % 4;
    }

    for (int i = 0; i < spaces; i++)
    {
        if (col != 0 || line != 0)
        {
            move_left();
            erase();
        }
    }
}

void Editor::move_right(bool shift)
{
    if (!shift && is_selection)
    {
        is_selection = false;

        if (sel_line > line || (sel_line == line && sel_col > col))
        {
            line = sel_line;
            col = sel_col;
        }

        return;
    }

    if (shift && !(line == text->size - 1 && col == text->lines[line].size))
        start_selection();

    vertical_move = false;

    if (col < text->lines[line].size)
        col++;
    else if (line != text->size - 1)
    {
        line++;
        col = 0;
    }
}

void Editor::move_left(bool shift)
{
    if (!shift && is_selection)
    {
        is_selection = false;

        if (sel_line < line || (sel_line == line && sel_col < col))
        {
            line = sel_line;
            col = sel_col;
        }

        return;
    }

    if (shift && !(line == 0 && col == 0))
        start_selection();

    vertical_move = false;

    if (col > 0)
        col--;
    else if (line > 0)
    {
        line--;
        col = text->lines[line].size;
    }
}

void Editor::move_up(bool shift)
{
    if (!shift && is_selection)
    {
        is_selection = false;

        if (sel_line < line || (sel_line == line && sel_col < col))
        {
            line = sel_line;
            col = sel_col;
        }
    }

    if (shift && line != 0)
        start_selection();

    if (!vertical_move)
    {
        vertical_move = true;
        target_col = col;
    }

    if (line > 0)
    {
        line--;
        col = target_col;

        if (col > text->lines[line].size)
            col = text->lines[line].size;
    }
}

void Editor::move_down(bool shift)
{
    if (!shift && is_selection)
    {
        is_selection = false;

        if (sel_line > line || (sel_line == line && sel_col > col))
        {
            line = sel_line;
            col = sel_col;
        }
    }

    if (shift && line != text->size - 1)
        start_selection();

    if (!vertical_move)
    {
        vertical_move = true;
        target_col = col;
    }

    if (line != text->size - 1)
    {
        line++;
        col = target_col;

        if (col > text->lines[line].size)
            col = text->lines[line].size;
    }
}

void Editor::move_to_line_start(bool shift)
{
    if (col > 0)
    {
        if (shift)
            start_selection();
        else
            is_selection = false;

        vertical_move = false;
        col = 0;
    }
}

void Editor::move_to_line_end(bool shift)
{
    if (col < text->lines[line].size)
    {
        if (shift)
            start_selection();
        else
            is_selection = false;

        vertical_move = false;
        col = text->lines[line].size;
    }
}

void Editor::swap_up()
{
    if (line == 0)
        return;

    deselect();

    Line tmp;
    Line* current = &text->lines[line];
    Line* above = &text->lines[line - 1];

    tmp.from(current);
    current->from(above);
    above->from(&tmp);

    line--;
    dirty = true;
}

void Editor::swap_down()
{
    if (line == text->size - 1)
        return;

    deselect();

    Line tmp;
    Line* current = &text->lines[line];
    Line* below = &text->lines[line + 1];

    tmp.from(current);
    current->from(below);
    below->from(&tmp);

    line++;
    dirty = true;
}

void Editor::page_up()
{
    int height = get_height() - 1;

    if (height < 0)
        return;

    view_start_line -= height;

    if (view_start_line < 0)
        view_start_line = 0;

    line = view_start_line + height - 4 - 1;
    col = 0;
}

void Editor::page_down()
{
    int height = get_height() - 1;

    if (height < 0)
        return;

    view_start_line += height;

    if (view_start_line >= text->size)
        view_start_line = text->size - 4 - 1;

    line = view_start_line + 4;
    col = 0;
}

void Editor::start_selection()
{
    if (!is_selection)
    {
        is_selection = true;
        sel_line = line;
        sel_col = col;
    }
}

void Editor::select_all()
{
    if (text->size == 1 && text->lines[0].size == 0)
        return;

    is_selection = true;
    sel_line = 0;
    sel_col = 0;

    line = text->size - 1;
    col = text->lines[line].size;
}

void Editor::select_line()
{
    if (text->size == 1 && text->lines[0].size == 0)
        return;

    is_selection = true;
    sel_line = line;
    sel_col = 0;

    col = text->lines[line].size;
}

bool is_select_word_char(char c)
{
    return isalnum(c) || c == '_';
}

void Editor::select_word()
{
    if (text->size == 1 && text->lines[0].size == 0)
        return;

    if (col >= text->lines[line].size)
        return;

    if (!is_select_word_char(text->lines[line].data[col]))
        return;

    is_selection = true;
    sel_line = line;
    sel_col = col;

    while (sel_col > 0 && is_select_word_char(text->lines[line].data[sel_col - 1]))
        sel_col--;

    while (col < text->lines[line].size && is_select_word_char(text->lines[line].data[col]))
        col++;
}

void Editor::erase_selection()
{
    if (!is_selection)
        return;

    int start_line = line;
    int start_col = col;

    int end_line = sel_line;
    int end_col = sel_col;

    if (start_line > end_line || (start_line == end_line && start_col > end_col))
    {
        int t = start_line;
        start_line = end_line;
        end_line = t;

        t = start_col;
        start_col = end_col;
        end_col = t;
    }

    Line res;
    res.size = 0;

    for (int i = 0; i < start_col; i++)
        res.push_back(text->lines[start_line].data[i]);

    for (int i = end_col; i < text->lines[end_line].size; i++)
        res.push_back(text->lines[end_line].data[i]);

    for (int i = 0; i < end_line - start_line + 1; i++)
        text->erase(start_line);

    text->insert_empty(start_line);
    memcpy(text->lines + start_line, &res, sizeof(Line));

    is_selection = false;

    line = start_line;
    col = start_col;

    dirty = true;
}

void Editor::deselect()
{
    is_selection = false;
}

void Editor::eval_shell_command()
{
    if (is_selection)
    {
        if (sel_line != line)
            return;
    }
    else
    {
        select_line();

        if (!is_selection)
            return;
    }

    char cmd[MAX_LINE_SIZE + 1];
    const char* args[] =
    {
        "/mnt/bin/sh",
        "-c",
        cmd,
        nullptr
    };

    int start_col = col > sel_col ? sel_col : col;
    int len = col > sel_col ? col - sel_col : sel_col - col;

    memcpy(cmd, text->lines[line].data + start_col, len);
    cmd[len] = 0;

    int fds[2];

    if (pipe(fds) != 0)
        return;

    if (fork() == 0)
    {
        dup2(fds[1], STDOUT_FILENO);
        dup2(open("/dev/null", O_WRONLY, 0), STDERR_FILENO);

        close(fds[0]);
        close(fds[1]);

        execve(args[0], (char* const*)args, 0);
        exit(1);
    }
    else
    {
        close(fds[1]);

        erase_selection();

        char buf[1024];
        isize bytes_read;

        bool newline_seen = false;

        while ((bytes_read = read(fds[0], buf, sizeof(buf))) > 0)
        {
            for (isize i = 0; i < bytes_read; i++)
            {
                if (buf[i] != '\n')
                {
                    if (newline_seen)
                    {
                        insert_char(' ');
                        newline_seen = false;
                    }

                    insert_char(buf[i]);
                }
                else
                    newline_seen = true;
            }
        }

        close(fds[0]);

        wait(-1, 0, 0);
        set_raw();
    }
}

void Editor::cut()
{
    if (is_selection)
    {
        erase_selection();
        return;
    }
    else

        if (text->size == 1)
        {
            text->lines[0].size = 0;
            col = 0;
        }
        else
        {
            text->erase(line);

            if (line == text->size)
            {
                line--;
                col = text->lines[line].size;
            }
            else
                col = 0;
        }

    dirty = true;
}

void Editor::trim_trailing_whitespace()
{
    for (int i = 0; i < text->size; i++)
    {
        Line* ln = &text->lines[i];

        if (ln->size == 0)
            continue;

        int j = ln->size - 1;
        int removed = 0;

        while (j >= 0 && isspace(ln->data[j])) {
            removed++;
            j--;
        }

        ln->size -= removed;

        if (line == i && col > ln->size)
            col = ln->size;
    }
}

void Editor::trim_trailing_newlines()
{
    int count = 0;
    for (int i = text->size - 1; i >= 0; i--)
    {
        if (text->lines[i].size == 0)
            count++;
        else
            break;
    }

    if (count <= 1)
        return;

    for (int i = 0; i < count - 1; i++)
        text->size--;

    if (line >= text->size)
    {
        line = text->size - 1;
        col = text->lines[line].size;
    }
}

void Editor::append_newline_if_needed()
{
    if (get_size() == 0)
        return;

    if (text->lines[text->size - 1].size != 0)
        text->push_back_empty();
}

usize Editor::get_size()
{
    usize ret = 0;

    for (int line = 0; line < text->size; line++)
    {
        ret += text->lines[line].size;

        if (line != text->size - 1)
            ret++;
    }

    return ret;
}

void draw(Editor* ed);

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
    PAGE_UP,
    PAGE_DOWN,

    SHIFT_ARROW_LEFT,
    SHIFT_ARROW_RIGHT,
    SHIFT_ARROW_UP,
    SHIFT_ARROW_DOWN,
    SHIFT_HOME,
    SHIFT_END,

    ALT_ARROW_UP,
    ALT_ARROW_DOWN,

    CTRL_Q,
    CTRL_S,
    CTRL_A,
    CTRL_X,
    CTRL_L,
    CTRL_D,
    CTRL_E
};

bool getchar(char* ch)
{
    if (read(term_fd, ch, 1) == 1)
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

            if (seq[2] == '~' && seq[1] == '5')
                return PAGE_UP;

            if (seq[2] == '~' && seq[1] == '6')
                return PAGE_DOWN;

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
                else if (seq[2] == ';' && seq[3] == '3')
                {
                    switch (seq[4])
                    {
                    case 'A': return ALT_ARROW_UP;
                    case 'B': return ALT_ARROW_DOWN;
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
    else if (c == CTRL('D'))    return CTRL_D;
    else if (c == CTRL('E'))    return CTRL_E;
    else                return c;
}

void process_keys(Editor* ed)
{
    int key = get_key();

    if (!key)
        return;

    if (isprint(key) || key == TAB || key == ENTER)
        ed->insert(key);
    else
    {
        switch (key)
        {
        case CTRL_Q:            quit = true;                    break;
        case CTRL_S:            ed->save();                    break;

        case CTRL_A:            ed->select_all();               break;
        case CTRL_L:            ed->select_line();              break;
        case CTRL_D:            ed->select_word();              break;
        case ESC:               ed->deselect();                 break;

        case BACKSPACE:         ed->backspace();                break;
        case DELETE:            ed->erase();                    break;
        case CTRL_E:            ed->eval_shell_command();       break;
        case CTRL_X:            ed->cut();                      break;

        case ARROW_LEFT:        ed->move_left();                break;
        case ARROW_RIGHT:       ed->move_right();               break;
        case ARROW_UP:          ed->move_up();                  break;
        case ARROW_DOWN:        ed->move_down();                break;
        case HOME:              ed->move_to_line_start();       break;
        case END:               ed->move_to_line_end();         break;

        case SHIFT_ARROW_LEFT:  ed->move_left(true);            break;
        case SHIFT_ARROW_RIGHT: ed->move_right(true);           break;
        case SHIFT_ARROW_UP:    ed->move_up(true);              break;
        case SHIFT_ARROW_DOWN:  ed->move_down(true);            break;
        case SHIFT_HOME:        ed->move_to_line_start(true);   break;
        case SHIFT_END:         ed->move_to_line_end(true);     break;

        case ALT_ARROW_UP:      ed->swap_up();                  break;
        case ALT_ARROW_DOWN:    ed->swap_down();                break;

        case PAGE_UP:           ed->page_up();                  break;
        case PAGE_DOWN:         ed->page_down();                break;
        }
    }

    draw(ed);
}

struct Selection
{
    bool active;
    int start_line;
    int start_col;
    int end_line;
    int end_col;

    bool contains(int line, int col)
    {
        if (!active || line < start_line || line > end_line)
            return false;

        if (line == start_line && col < start_col)
            return false;

        if (line == end_line && col >= end_col)
            return false;

        return true;
    }
};

void get_selection(Editor* ed, Selection* sel)
{
    sel->active = ed->is_selection;

    if (!ed->is_selection)
        return;

    sel->start_line = ed->line;
    sel->start_col = ed->col;

    sel->end_line = ed->sel_line;
    sel->end_col = ed->sel_col;

    if (sel->start_line > sel->end_line || (sel->start_line == sel->end_line && sel->start_col > sel->end_col))
    {
        int t = sel->start_line;
        sel->start_line = sel->end_line;
        sel->end_line = t;

        t = sel->start_col;
        sel->start_col = sel->end_col;
        sel->end_col = t;
    }
}

int get_num_size(int num)
{
    if (num == 0)
        return 1;

    int ret = 0;

    while (num)
    {
        num /= 10;
        ret++;
    }

    return ret;
}

bool parse_whitespace(char*& ptr)
{
    if (!isspace(*ptr))
        return false;

    while (isspace(*ptr))
        ptr++;

    return true;
}

bool parse_comment(char*& ptr)
{
    if (*ptr != '/' || *(ptr + 1) != '/')
        return false;

    while (*ptr)
        ptr++;

    return true;
}

bool match_word(char* ptr, char* end, const char* word)
{
    char* cpy = ptr;

    while (*word)
    {
        if (cpy == end || *cpy != *word)
            return false;

        cpy++;
        word++;
    }

    if (cpy != end)
        return false;

    ptr = cpy;

    return true;
}

const char* keywords_ctrl[] =
{
    "break", "case", "continue", "default", "do", "else", "for",
    "goto", "if", "return", "switch", "while",
};

const char* keywords_data[] =
{
    "alignas", "alignof", "auto", "bool", "char", "const", "constexpr", "double",
    "enum", "extern", "float", "inline", "int", "long", "register", "restrict",
    "short", "signed", "sizeof", "static", "struct", "typedef", "union",
    "unsigned", "void", "volatile",

    "u8", "u16", "u32", "u64", "usize",
    "i8", "i16", "i32", "i64", "isize",
    "f32", "f64",
};

const char* literals[] = { "true", "false", "NULL", "nullptr" };

bool match_keyword_ctrl(char* str, char* end)
{
    for (int i = 0; i < sizeof(keywords_ctrl) / sizeof(keywords_ctrl[0]); i++)
        if (match_word(str, end, keywords_ctrl[i]))
            return true;

    return false;
}

bool match_keyword_data(char* str, char* end)
{
    for (int i = 0; i < sizeof(keywords_data) / sizeof(keywords_data[0]); i++)
        if (match_word(str, end, keywords_data[i]))
            return true;

    return false;
}

bool match_literal(char* str, char* end)
{
    for (int i = 0; i < sizeof(literals) / sizeof(literals[0]); i++)
        if (match_word(str, end, literals[i]))
            return true;

    return false;
}

bool parse_ident(char*& ptr)
{
    if (!isalpha(*ptr) && *ptr != '_')
        return false;

    while (isalnum(*ptr) || *ptr == '_')
        ptr++;

    return true;
}

bool has_inc_directive;
bool parse_directive(char*& ptr)
{
    if (*ptr != '#')
        return false;

    char* cpy = ptr;
    cpy++;

    if (!parse_ident(cpy))
        return false;

    if (match_word(ptr, cpy, "#include"))
        has_inc_directive = true;

    ptr = cpy;

    return true;
}

bool parse_include_path(char*& ptr)
{
    if (!has_inc_directive || *ptr != '<')
        return false;

    while (*ptr && *ptr != '>')
        ptr++;

    if (*ptr)
        ptr++;

    has_inc_directive = false;
    return true;
}

bool parse_string(char*& ptr)
{
    if (*ptr != '"' && *ptr != '\'')
        return false;

    char ch = *ptr++;

    while (*ptr && (ptr[-1] == '\\' || *ptr != ch))
        ptr++;

    if (*ptr)
        ptr++;

    return true;
}

bool parse_hex(char*& ptr)
{
    if (*ptr != '0' || (*(ptr + 1) != 'x') && (*(ptr + 1) != 'X'))
        return false;

    char* cpy = ptr + 2;

    if (!isxdigit(*cpy))
        return false;

    while (*cpy && isxdigit(*cpy))
        cpy++;

    ptr = cpy;

    return true;
}

bool parse_number(char*& ptr)
{
    if (!isdigit(*ptr) && *ptr != '-')
        return false;

    char* cpy = ptr;

    if (*cpy == '-')
        cpy++;

    if (!isdigit(*cpy))
        return false;

    while (*cpy && isdigit(*cpy))
        cpy++;

    ptr = cpy;
    return true;
}

bool parse_literal(char*& ptr)
{
    return parse_hex(ptr) || parse_number(ptr);
}

bool parse_punctuation(char*& ptr)
{
    if (strchr(".,;", *ptr))
    {
        ptr++;
        return true;
    }

    if (*ptr == '-' && *(ptr + 1) == '>')
    {
        ptr += 2;
        return true;
    }

    if (*ptr == ':' && *(ptr + 1) == ':')
    {
        ptr += 2;
        return true;
    }

    return false;
}

bool parse_braces(char*& ptr)
{
    if (strchr("{}[]()", *ptr))
    {
        ptr++;
        return true;
    }

    return false;
}

Token next_c_token(char*& ptr)
{
    if (*ptr == 0)
    {
        has_inc_directive = false;
        return { ptr, 0, 39 };
    }

    Token ret;
    ret.start = ptr;

    if (parse_whitespace(ptr))
        ret.color = 39;
    else if (parse_comment(ptr))
        ret.color = 37;
    else if (parse_directive(ptr))
        ret.color = 94;
    else if (parse_include_path(ptr))
        ret.color = 92;
    else if (parse_string(ptr))
        ret.color = 92;
    else if (parse_literal(ptr))
        ret.color = 95;
    else if (parse_braces(ptr))
        ret.color = 34;
    else
    {
        char* cpy = ptr;

        if (parse_ident(cpy))
        {
            if (match_keyword_ctrl(ptr, cpy))
                ret.color = 91;
            else if (match_keyword_data(ptr, cpy))
                ret.color = 93;
            else if (match_literal(ptr, cpy))
                ret.color = 95;
            else
                ret.color = 39;

            ptr = cpy;
        }
        else if (parse_punctuation(ptr))
            ret.color = 37;
        else
        {
            // in this case we have stuff like +, -, ?, :, etc
            ptr++;
            ret.color = 96;
        }
    }

    ret.size = ptr - ret.start;

    return ret;
}

const char* keywords_asm[] =
{
    "section", "global", "extern", "align", "bits",
    "db", "dw", "dd", "dq", "dt",
    "resb", "resw", "resd", "resq",
    "byte", "word", "dword", "qword", "ptr",
};

const char* registers[] =
{
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",

    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",

    "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",

    "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
    "ah", "bh", "ch", "dh",

    "cs", "ds", "es", "fs", "gs", "ss",
};

const char* instructions[] =
{
    // Data Transfer Instructions
    "mov", "movsx", "movzx", "push", "pushf", "pushfq", "pop", "popf", "popfq",
    "xchg", "xlat", "lea",

    // Arithmetic Instructions
    "add", "adc", "sub", "sbb", "imul", "idiv", "inc", "dec", "neg", "cmp", "cmpxchg",

    // Logical Instructions
    "and", "or", "xor", "not", "test",

    // Shift and Rotate Instructions
    "shl", "shr", "sal", "sar", "rol", "ror", "rcl", "rcr",

    // Control Transfer Instructions
    "jmp", "je", "jne", "jg", "jge", "jl", "jle", "ja", "jae", "jb", "jbe", "jo", "jno",
    "js", "jns", "jc", "jnc", "jz", "jnz", "jp", "jnp", "call", "ret", "retn", "retf",
    "enter", "leave", "loop", "loope", "loopne",

    // Interrupt and System Instructions
    "int", "int3", "iret", "iretq", "syscall", "sysret",

    // String Instructions
    "movs", "movsb", "movsw", "movsd", "movsq", "cmps", "cmpsb", "cmpsw", "cmpsd", "cmpsq",
    "scas", "scasb", "scasw", "scasd", "scasq", "lods", "lodsb", "lodsw", "lodsd", "lodsq",
    "stos", "stosb", "stosw", "stosd", "stosq", "rep", "repe", "repne", "repnz", "repz",

    // Bit and Byte Instructions
    "bt", "bts", "btr", "btc", "bsf", "bsr", "setb", "setae", "sete", "setne", "setbe", "seta",
    "sets", "setns", "setp", "setnp", "setl", "setge", "setle", "setg",

    // Flag Control Instructions
    "clc", "cld", "cli", "clts", "cmc", "stc", "std", "sti",

    // Privileged Instructions
    "lidt", "lgdt", "sidt", "sgdt", "ltr", "lldt", "str",
    "cli", "sti", "clts", "hlt", "invlpg", "wbinvd", "invd",
    "cpuid", "rdmsr", "wrmsr", "rdtsc", "rdtscp", "swapgs",
    "clflush", "lfence", "mfence", "sfence", "rdpmc", "rsm",
    "xsave", "xrstor", "xsaveopt", "wrxsr", "clac", "stac",
    "monitor", "mwait", "vmcall", "vmclear", "vmlaunch",
    "vmresume", "vmptrld", "vmptrst", "vmxoff", "vmxon",
    "skinit", "invlpga", "rdpid", "rdrand", "rdseed",

    // Miscellaneous Instructions
    "nop", "pause", "ud2", "cpuid", "in", "ins", "insb", "insw", "insd", "out", "outs",
    "outsb", "outsw", "outsd", "hlt", "wait", "xlatb", "cwd", "cdq", "cqo", "cbw",
    "cwde", "cdqe", "lahf", "sahf"
};

bool match_keyword_asm(char* ptr, char* end)
{
    for (int i = 0; i < sizeof(keywords_asm) / sizeof(keywords_asm[0]); i++)
        if (match_word(ptr, end, keywords_asm[i]))
            return true;

    return false;
}

bool match_register(char* ptr, char* end)
{
    for (int i = 0; i < sizeof(registers) / sizeof(registers[0]); i++)
        if (match_word(ptr, end, registers[i]))
            return true;

    return false;
}

bool match_instruction(char* ptr, char* end)
{
    for (int i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++)
        if (match_word(ptr, end, instructions[i]))
            return true;

    return false;
}

bool parse_asm_directive(char*& ptr)
{
    if (*ptr != '%')
        return false;

    char* cpy = ptr;
    cpy++;

    if (!parse_ident(cpy))
        return false;

    ptr = cpy;

    return true;
}

bool parse_label(char*& ptr)
{
    if (!isalpha(*ptr) && *ptr != '_' && *ptr != '.')
        return false;

    char* cpy = ptr;

    while (isalnum(*cpy) || *cpy == '_' || *cpy == '.')
        cpy++;

    if (*cpy == ':')
    {
        ptr = cpy + 1;
        return true;
    }

    return false;
}

bool parse_asm_comment(char*& ptr)
{
    if (*ptr != ';')
        return false;

    while (*ptr)
        ptr++;

    return true;
}

Token next_asm_token(char*& ptr)
{
    if (*ptr == 0)
        return { ptr, 0, 39 };

    Token ret;
    ret.start = ptr;

    if (parse_whitespace(ptr))
        ret.color = 39;
    else if (parse_asm_comment(ptr))
        ret.color = 37;
    else if (parse_asm_directive(ptr))
        ret.color = 94;
    else if (parse_string(ptr))
        ret.color = 92;
    else if (parse_literal(ptr))
        ret.color = 92;
    else
    {
        char* cpy = ptr;
        char* cpy2 = ptr;

        if (parse_label(cpy) && !match_register(ptr, cpy - 1))
        {
            ptr = cpy;
            ret.color = 96;
        }
        else if (parse_ident(cpy2))
        {
            if (match_register(ptr, cpy2))
                ret.color = 93;
            else if (match_keyword_asm(ptr, cpy2))
                ret.color = 94;
            else if (match_instruction(ptr, cpy2))
                ret.color = 91;
            else
                ret.color = 39;

            ptr = cpy2;
        }
        else
        {
            ptr++;
            ret.color = 39;
        }
    }

    ret.size = ptr - ret.start;

    return ret;
}

Token next_regular_token(char*& ptr)
{
    if (*ptr == 0)
        return { ptr, 0, 39 };

    Token ret;
    ret.start = ptr;

    if (parse_whitespace(ptr))
        ret.color = 39;
    else
    {
        while (*ptr && !isspace(*ptr))
            ptr++;

        ret.color = 39;
    }

    ret.size = ptr - ret.start;

    return ret;
}

void highlight(Line* ln, Token(*next_token)(char*&))
{
    char* ptr = ln->data;
    int i = 0;
    Token tok;

    while (true)
    {
        tok = next_token(ptr);

        if (tok.size == 0)
            break;

        for (int j = 0; j < tok.size; j++)
            color_buf[i++] = tok.color;
    }

    color_buf[i] = tok.color;
}

void draw(Editor* ed)
{
    int height = get_height() - 1;
    int width = get_width();

    int top_margin = ed->line - 4;
    int bottom_margin = ed->line + 4;

    if (top_margin < 0)
        top_margin = 0;

    if (view_start_line > top_margin)
        view_start_line = top_margin;
    else if (view_start_line < bottom_margin - height + 1)
        view_start_line = bottom_margin - height + 1;

    int min_size = get_num_size(ed->text->size);

    if (min_size < 4)
        min_size = 4;

    Selection sel;
    get_selection(ed, &sel);

    for (int i = 0; i < height; i++)
    {
        move_cursor(i, 0);

        int line = view_start_line + i;

        // line doesn't exist so print a tilde
        if (line >= ed->text->size)
        {
            printf("\e[34m%*s\e[m\e[K", min_size, "~");
            continue;
        }

        Line* ln = &ed->text->lines[line];
        ln->data[ln->size] = 0; // null terminate the line

        highlight(ln, ed->next_token);

        // print line number and highlight the current line
        printf("\e[%dm%*d\e[m  ", line == ed->line ? 96 : 34, min_size, line + 1);

        int col;
        bool in_reverse = false;
        int color = -1;

        for (col = 0; col <= ln->size; col++)
        {
            char ch = col < ln->size ? ln->data[col] : ' ';
            bool at_cursor = line == ed->line && col == ed->col;
            bool should_reverse = !at_cursor && sel.contains(line, col);

            if (color_buf[col] != color)
            {
                color = color_buf[col];
                printf("\e[%dm", color);
            }

            if (should_reverse != in_reverse)
            {
                in_reverse = should_reverse;
                printf(in_reverse ? "\e[100m" : "\e[49m");
            }

            printf(at_cursor ? "\e[7m%c\e[27m" : "%c", ch);
        }

        // clear screen from cursor until the end of line
        printf("\e[m\e[K");
    }

    // paint the status bar
    move_cursor(height, 0);
    printf("\e[97;44m\e[K");

    const char* name = ed->filename ? ed->filename : "\e[90m[temp]\e[39m";
    const char* status = !ed->filename ? "" : (ed->dirty ? "*" : " ");
    move_cursor(height, 4);
    printf("%s%s     %lu bytes", name, status, ed->get_size());

    move_cursor(height, width - 16);
    printf("%d,%d\e[m", ed->line + 1, ed->col + 1);

    flush();
}

int ed(char* filename, int p_flag)
{
    term_fd = open("/dev/fbterm", O_RDONLY, 0);

    if (term_fd < 0)
    {
        write(2, "ed: could not open /dev/fbterm\n", 31);
        return 1;
    }

    output_fd = term_fd;
    Editor ed(filename);

    set_raw();
    draw(&ed);

    while (!quit)
        process_keys(&ed);

    clear_screen();
    set_cooked();

    if (p_flag)
        ed.write_to_fd(STDOUT_FILENO);

    return 0;
}

void print_usage()
{
    write(2, "usage: ed [-p] [file]\n", 22);
}

int main(int argc, char** argv)
{
    if (argc == 1)
        return ed(nullptr, false);

    // no flag, so filename
    if (argv[1][0] != '-')
        return ed(argv[1], false);

    if (strcmp(argv[1], "-p") == 0)
        return ed(argc > 2 ? argv[2] : nullptr, true);

    print_usage();
    return 1;
}
