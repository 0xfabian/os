#include <kbd.h>
#include <task.h>

KeyQueue key_queue;
Task* kbd_task;
u8 kbd_state;

void KeyQueue::push(char key)
{
    // this could happen if the task hasn't spawned yet
    // so just ignore the key press
    if (!kbd_task)
        return;

    if (empty() && kbd_task->state == TASK_SLEEPING)
        kbd_task->ready();

    buffer[tail++] = key;
}

char KeyQueue::pop()
{
    char key = buffer[head++];
    return key;
}

bool KeyQueue::empty()
{
    return head == tail;
}

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

void keyboard_task()
{
    // we need to clear the keyboard buffer
    // because sometimes the buffer is not empty on kernel entry
    // and so it won't generate interrupts
    if (inb(0x64) & 1)
        inb(0x60);

    // e0 53 delete     \e[3~
    // e0 47 home       \e[H
    // e0 4f end        \e[F
    // e0 48 up         \e[A
    // e0 50 down       \e[B
    // e0 4d right      \e[C
    // e0 4b left       \e[D
    // 1;2 shift modifier

    while (true)
    {
        cli();

        if (!key_queue.empty())
        {
            u8 key = key_queue.pop();
            bool extended = false;

            if (key == 0xe0)
            {
                extended = true;
                key = key_queue.pop();
            }

            if (extended && (key & 0x80) == 0)
            {
                if (fbterm.echo)
                {
                    fbterm.receive_char('^');
                    fbterm.receive_char('[');
                }
                else
                    fbterm.receive_char('\e');

                fbterm.receive_char('[');

                if (key == 0x53)
                {
                    fbterm.receive_char('3');
                    fbterm.receive_char('~');
                }
                else
                {
                    if (kbd_state & KBD_LSHIFT || kbd_state & KBD_RSHIFT)
                    {
                        fbterm.receive_char('1');
                        fbterm.receive_char(';');
                        fbterm.receive_char('2');
                    }

                    if (key == 0x47)
                        fbterm.receive_char('H');
                    else if (key == 0x4f)
                        fbterm.receive_char('F');
                    else if (key == 0x48)
                        fbterm.receive_char('A');
                    else if (key == 0x50)
                        fbterm.receive_char('B');
                    else if (key == 0x4d)
                        fbterm.receive_char('C');
                    else if (key == 0x4b)
                        fbterm.receive_char('D');
                }
            }
            else
            {
                char ch = translate_key(key);

                if (ch == 'd' && kbd_state & KBD_CTRL)
                    fbterm.handle_requests();
                else if (ch == 'u' && kbd_state & KBD_CTRL)
                    fbterm.clear_input();
                else if (ch)
                    fbterm.receive_char(ch);
            }
        }
        else
            pause();

        sti();
    }
}