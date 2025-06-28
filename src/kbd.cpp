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
    else if (base == 0x38)
        mask = KBD_ALT;

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

void wait_input_buffer_empty()
{
    while (inb(0x64) & 2);
}

void wait_output_buffer_full()
{
    while (!(inb(0x64) & 1));
}

void send_command(u8 command)
{
    wait_input_buffer_empty();
    outb(0x60, command);
}

u8 read_response()
{
    wait_output_buffer_full();
    return inb(0x60);
}

bool set_typematic(u8 typematic)
{
    send_command(0xf3);
    u8 resp = read_response();

    if (resp != 0xfa)
        return false;

    send_command(typematic);
    resp = read_response();

    if (resp != 0xfa)
        return false;

    return true;
}

void keyboard_task()
{
    cli();

    // we need to clear the keyboard buffer
    // because sometimes the buffer is not empty on kernel entry
    // and so it won't generate interrupts
    if (inb(0x64) & 1)
        inb(0x60);

    if (!set_typematic(0b00100000))
        kprintf(WARN "Failed to set keyboard typematic\n");

    sti();

    // 16    u
    // 20    d
    // e0 53 delete     \e[3~
    // e0 49 page up    \e[5~
    // e0 51 page down  \e[6~
    // e0 47 home       \e[H
    // e0 4f end        \e[F
    // e0 48 up         \e[A
    // e0 50 down       \e[B
    // e0 4d right      \e[C
    // e0 4b left       \e[D
    // 1;2 shift modifier
    // 1;3 alt modifier

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

            if (extended)
            {
                if ((key & 0x80) == 0)
                {
                    char buf[16];
                    char* end = buf;

                    if (fbterm.echo)
                    {
                        *end++ = '^';
                        *end++ = '[';
                    }
                    else
                        *end++ = '\e';

                    *end++ = '[';

                    if (key == 0x53) // delete
                    {
                        *end++ = '3';
                        *end++ = '~';
                    }
                    else if (key == 0x49) // page up
                    {
                        *end++ = '5';
                        *end++ = '~';
                    }
                    else if (key == 0x51) // page down
                    {
                        *end++ = '6';
                        *end++ = '~';
                    }
                    else
                    {
                        if (kbd_state & KBD_LSHIFT || kbd_state & KBD_RSHIFT)
                        {
                            *end++ = '1';
                            *end++ = ';';
                            *end++ = '2';
                        }
                        else if (kbd_state & KBD_ALT)
                        {
                            *end++ = '1';
                            *end++ = ';';
                            *end++ = '3';
                        }

                        switch (key)
                        {
                        case 0x47:      *end++ = 'H';       break; // home
                        case 0x4f:      *end++ = 'F';       break; // end
                        case 0x48:      *end++ = 'A';       break; // up
                        case 0x50:      *end++ = 'B';       break; // down
                        case 0x4d:      *end++ = 'C';       break; // right
                        case 0x4b:      *end++ = 'D';       break; // left
                        default:        end = buf;          break;
                        }
                    }

                    for (char* ch = buf; ch < end; ch++)
                        fbterm.receive_char(*ch);
                }
            }
            else
            {
                char ch = translate_key(key);

                if (key == 0x2e && kbd_state & KBD_CTRL)
                {
                    int ret = exit_group(fbterm.fg_group);

                    if (ret > 0)
                        kprintf("\a\e[37mKilled \e[91m%d\e[37m foreground task%c\e[m\n", ret, ret > 1 ? 's' : 0);
                }
                else if (fbterm.line_buffered && key == 0x20 && kbd_state & KBD_CTRL)
                {
                    // this is a hacky way to implement EOF, what if, after we wake up the readers
                    // we receive more input before any of the readers actually continue the read syscall
                    // then when some task will continue the read it will get the extra input
                    // this is a bug, it should only get the input until EOF
                    fbterm.eof_received = true;
                    fbterm.readers_queue.wake_all();
                }
                else if (fbterm.line_buffered && key == 0x16 && kbd_state & KBD_CTRL)
                {
                    fbterm.clear_input();
                }
                else if (ch)
                {
                    if (isalpha(ch) && kbd_state & KBD_CTRL)
                    {
                        if (ch >= 'a')
                            ch = ch - 'a' + 'A';

                        if (fbterm.echo)
                        {
                            fbterm.receive_char('^');
                            fbterm.receive_char(ch);
                        }
                        else
                            fbterm.receive_char(ch - 'A' + 1);
                    }
                    else if (ch == '\b' && !fbterm.line_buffered)
                        fbterm.receive_char(0x7f);
                    else
                        fbterm.receive_char(ch);
                }
            }
        }
        else
            pause();

        sti();
    }
}
