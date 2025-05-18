#pragma once

#include <error.h>
#include <mem/heap.h>
#include <waitq.h>

#define PIPE_PAGES  1
#define PIPE_SIZE   (PIPE_PAGES * PAGE_SIZE)

struct File;

struct Pipe
{
    u8* buffer;
    usize data_size;

    usize read_pos;
    usize write_pos;

    File* read_end;
    File* write_end;

    WaitQueue readers_queue;
    WaitQueue writers_queue;

    static result_ptr<Pipe> open();
    void close();
};