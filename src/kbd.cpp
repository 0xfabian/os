#include <kbd.h>
#include <task.h>

u8 kbd_state;

char key_lookup[128] =
{
    0, '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, '*', 0, ' ',

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0,

    '\\'
};

char shift_key_lookup[128] =
{
    0, '\e', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, '*', 0, ' ',

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0,

    '|'
};

char translate_key(int key)
{
    if (key == 0x3a)
    {
        kbd_state ^= KBD_CAPS;
        return 0;
    }

    u8 base = key & 0x7f;
    u8 mask = 0;

    if (base == 0x1d)
        mask = KBD_CTRL;
    else if (base == 0x2a)
        mask = KBD_LSHIFT;
    else if (base == 0x36)
        mask = KBD_RSHIFT;

    if (mask)
    {
        if (key & 0x80)
            kbd_state &= ~mask;
        else
            kbd_state |= mask;

        return 0;
    }

    if (key & 0x80)
        return 0;

    bool shift = kbd_state & KBD_LSHIFT || kbd_state & KBD_RSHIFT;
    char ch = shift ? shift_key_lookup[key] : key_lookup[key];

    if ((kbd_state & KBD_CAPS) && isalpha(ch))
        ch += shift ? 32 : -32;

    return ch;
}