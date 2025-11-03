#include <user.h>
#include <qsort.h>

#define ENTRY_NAME_MAX  64
#define BUF_SIZE        1024

struct Entry
{
    char name[ENTRY_NAME_MAX + 1];
    int len;
};

struct Entry* entries;
int count;
int capacity = 256;

int sbrk(usize size)
{
    u64 current = (u64)brk(NULL);
    u64 new = current + size;

    brk((void*)new);
}

int entry_cmp(const void* a, const void* b)
{
    const struct Entry* ea = a;
    const struct Entry* eb = b;

    return strcmp(ea->name, eb->name);
}

void init_entries()
{
    entries = (struct Entry*)brk(NULL);
    sbrk(capacity * sizeof(struct Entry));
    memset(entries, 0, capacity * sizeof(struct Entry));
    count = 0;
}

inline int entry_push_back(struct Entry* ent, char c)
{
    if (ent->len >= ENTRY_NAME_MAX)
        return 0;

    ent->name[ent->len++] = c;
    return 1;
}

inline void entry_finish(struct Entry* ent)
{
    ent->name[ent->len] = 0;
    count++;

    if (count == capacity)
    {
        int more = capacity;
        sbrk(more * sizeof(struct Entry));
        memset(entries + capacity, 0, more * sizeof(struct Entry));
        capacity += more;
    }
}

void read_entries(int fd)
{
    char buf[BUF_SIZE];
    int bytes_read;

    while ((bytes_read = read(fd, buf, BUF_SIZE)) > 0)
    {
        for (isize i = 0; i < bytes_read; i++)
        {
            if (buf[i] == '\n')
                entry_finish(&entries[count]);
            else
                entry_push_back(&entries[count], buf[i]);
        }
    }

    if (bytes_read < 0)
    {
        write(2, "mc: read failed\n", 16);
        exit(1);
    }
}

int valid_count2(int start, int count, int cols)
{
    if (start > count)
        return 0;

    if (start == count)
        return 1;

    return valid_count2(start + cols - 1, count, cols);
}

int valid_count(int start, int count, int cols)
{
    if (start > count)
        return 0;

    if (start == count)
        return 1;

    return valid_count(start + cols, count, cols) || valid_count2(start + cols - 1, count, cols);
}

int check_fit(struct Entry* entries, usize count, int cols, int width)
{
    if (count < cols)
        return 0;

    if (!valid_count(cols, count, cols))
        return 0;

    int lines = count / cols + (count % cols > 0);

    int necessary_width = 0;
    int index = 0;

    for (int i = 0; i < cols; i++)
    {
        int max_name_in_col = 0;

        for (int j = 0; j < lines; j++)
        {
            if (index >= count)
                break;

            if (entries[index].len > max_name_in_col)
                max_name_in_col = entries[index].len;

            index++;
        }

        necessary_width += max_name_in_col + 2;
    }

    return necessary_width <= (width + 1);
}

void write_entries_formatted(int fd)
{
    int cols = 80;

    if (isatty(fd))
        ioctl(fd, FBTERM_GET_WIDTH, &cols);

    int good = 1;

    for (int i = 2; i <= cols; i++)
        if (check_fit(entries, count, i, cols))
            good = i;

    cols = good;

    int lines = count / cols + (count % cols > 0);
    int index = 0;

    int max_widths[cols];

    for (int i = 0; i < cols; i++)
    {
        int max_name_in_col = 0;

        for (int j = 0; j < lines; j++)
        {
            if (index >= count)
                break;

            if (entries[index].len > max_name_in_col)
                max_name_in_col = entries[index].len;

            index++;
        }

        max_widths[i] = max_name_in_col;
    }

    auto_flush = false;

    for (int i = 0; i < lines; i++)
    {
        int offset = i;

        int col = 0;
        while (offset < count)
        {
            printf("%*s", -max_widths[col], entries[offset].name);

            if (col < cols - 1)
                printf("  ");

            offset += lines;
            col++;
        }

        printf("\n");
    }

    flush();
}

int main(int argc, char** argv)
{
    init_entries();

    read_entries(STDIN_FILENO);

    qsort(entries, count, sizeof(struct Entry), entry_cmp);

    write_entries_formatted(STDOUT_FILENO);

    return 0;
}
