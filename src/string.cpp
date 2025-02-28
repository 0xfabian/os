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