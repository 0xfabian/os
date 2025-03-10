#pragma once

#include <types.h>

#define KBD_CTRL    1
#define KBD_LSHIFT  2
#define KBD_RSHIFT  4
#define KBD_CAPS    8

struct KeyEventQueue
{
    u8 buffer[256];
    int head = 0;
    int tail = 0;

    void push(u8 scancode)
    {
        buffer[tail] = scancode;
        tail = (tail + 1) & 0xff;
    }

    bool pop(u8* scancode)
    {
        if (head == tail)
            return false;

        *scancode = buffer[head];
        head = (head + 1) & 0xff;

        return true;
    }
};

extern KeyEventQueue key_queue;
extern char key_lookup[128];
extern char shift_key_lookup[128];
extern u8 kbd_state;

static inline bool isalpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool isprint(char c)
{
    return c >= ' ' && c <= '~';
}

char translate_key(int key);
void keyboard_task();