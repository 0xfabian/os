#pragma once

#include <types.h>

#define KBD_CTRL    1
#define KBD_LSHIFT  2
#define KBD_RSHIFT  4
#define KBD_CAPS    8
#define KBD_ALT     16

struct KeyQueue
{
    u8 buffer[256];
    u8 head;
    u8 tail;

    void push(char key);
    char pop();
    bool empty();
};

struct Task;

extern KeyQueue key_queue;
extern Task* kbd_task;
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
