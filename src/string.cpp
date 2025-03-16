#include <string.h>
#include <mem/heap.h>

usize strlen(const char* str)
{
    usize len = 0;

    while (str[len])
        len++;

    return len;
}

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && *s1 == *s2)
    {
        s1++;
        s2++;
    }

    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, usize n)
{
    while (n && *s1 && *s1 == *s2)
    {
        s1++;
        s2++;
        n--;
    }

    return n ? *(unsigned char*)s1 - *(unsigned char*)s2 : 0;
}

char* strcpy(char* dest, const char* src)
{
    usize len = strlen(src);

    memcpy((void*)dest, (void*)src, len + 1);

    return dest;
}

char* strdup(const char* str)
{
    usize len = strlen(str);

    char* dup = (char*)kmalloc(len + 1);

    if (!dup)
        return nullptr;

    memcpy(dup, str, len + 1);

    return dup;
}

char* strcat(char* dest, const char* src)
{
    usize len = strlen(dest);

    strcpy(dest + len, src);

    return dest;
}

#define PATH_MAX 256
char path_read_data[PATH_MAX];

// hacky path reading function
char* path_read_next(const char*& ptr)
{
    while (*ptr == '/')
        ptr++;

    if (*ptr == '\0')
        return nullptr;

    char* output = path_read_data;
    usize len = 0;

    while (*ptr != '/' && *ptr != '\0')
    {
        if (len >= PATH_MAX - 1)
        {
            kprintf(WARN "path_read_next(): path too long\n");
            break;
        }

        output[len++] = *ptr++;
    }

    output[len] = '\0';

    return path_read_data;
}

// this should be always an absolute path
char* normalize_path(const char* path)
{
    if (!path)
    {
        kprintf(WARN "normalize_path: path is null\n");
        return nullptr;
    }

    char* normalized = (char*)kmalloc(strlen(path) + 1);
    *normalized = 0;

    strcat(normalized, "/");

    char* name;
    while ((name = path_read_next(path)))
    {
        if (strcmp(name, ".") == 0)
            continue;

        if (strcmp(name, "..") == 0)
        {
            usize len = strlen(normalized);

            // can't go back from root
            if (len == 1)
                continue;

            for (int i = len - 1; i >= 1; i--)
            {
                if (normalized[i] == '/')
                {
                    normalized[i] = 0;
                    break;
                }

                normalized[i] = 0;
            }
        }
        else
        {
            if (normalized[strlen(normalized) - 1] != '/')
                strcat(normalized, "/");

            strcat(normalized, name);
        }
    }

    return normalized;
}