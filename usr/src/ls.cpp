#include <user.h>
#include <qsort.h>

struct Dirent
{
    unsigned long ino;
    unsigned int type;
    char name[32];
};

#define MAX_DENTS 256

Dirent dents[MAX_DENTS];
usize count;

int compare(const void* a, const void* b)
{
    Dirent* da = (Dirent*)a;
    Dirent* db = (Dirent*)b;

    return strcmp(da->name, db->name);
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

int check_fit(Dirent* entries, usize count, int cols, int width)
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

            int name_len = strlen(entries[index].name);

            if (name_len > max_name_in_col)
                max_name_in_col = name_len;

            index++;
        }

        necessary_width += max_name_in_col + 2;
    }

    return necessary_width <= (width + 1);
}

char full_path[256];
int reset_index = 0;

void print_dents_simple(Dirent* entries, usize count)
{
    for (usize i = 0; i < count; i++)
        printf("%s\n", entries[i].name);
}

void print_dents(Dirent* entries, usize count, int cols)
{
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

            int name_len = strlen(entries[index].name);

            if (name_len > max_name_in_col)
                max_name_in_col = name_len;

            index++;
        }

        max_widths[i] = max_name_in_col;
    }

    for (int i = 0; i < lines; i++)
    {
        int offset = i;

        int col = 0;
        while (offset < count)
        {
            full_path[reset_index] = 0;
            strcat(full_path, entries[offset].name);

            const char* color = "39";

            struct stat st;
            if (lstat(full_path, &st) == 0)
            {
                int type = st.st_mode & IT_MASK;
                int perm = st.st_mode & IP_MASK;

                if (type == IT_DIR)
                    color = "94";
                else if (type == IT_CDEV || type == IT_BDEV)
                    color = "93";
                else if (type == IT_LINK)
                    color = "96";
                else if (perm & IP_X)
                    color = "92";
            }

            printf("\e[%sm%*s\e[m", color, -max_widths[col], entries[offset].name);

            if (col < cols - 1)
                printf("  ");

            offset += lines;
            col++;
        }

        printf("\n");
    }
}

void ls(const char* path)
{
    int fd = open(path, O_DIRECTORY, 0);

    if (fd < 0)
    {
        write(1, "ls: open failed\n", 16);
        return;
    }

    strcpy(full_path, path);
    strcat(full_path, "/");
    reset_index = strlen(full_path);

    auto_flush = false;

    long bytes_read = 0;

    while ((bytes_read = getdents(fd, dents + count, (MAX_DENTS - count) * sizeof(Dirent))) > 0)
        count += bytes_read / sizeof(Dirent);

    qsort(dents, count, sizeof(Dirent), compare);

    if (isatty(STDOUT_FILENO))
    {
        int cols = 80;
        ioctl(STDOUT_FILENO, FBTERM_GET_WIDTH, &cols);

        int good = 1;

        for (int i = 2; i <= cols; i++)
            if (check_fit(dents, count, i, cols))
                good = i;

        print_dents(dents, count, good);
    }
    else
        print_dents_simple(dents, count);

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
