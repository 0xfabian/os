#pragma once

#include <types.h>

#define KBD_CTRL    1
#define KBD_LSHIFT  2
#define KBD_RSHIFT  4
#define KBD_CAPS    8

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