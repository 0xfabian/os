#include <pipe.h>
#include <fs/file.h>
#include <task.h>

extern FileOps pipe_read_end_fops;
extern FileOps pipe_write_end_fops;

result_ptr<Pipe> Pipe::open()
{
    Pipe* pipe = (Pipe*)kmalloc(sizeof(Pipe));

    if (!pipe)
        return -ERR_NO_MEM;

    pipe->data_size = 0;
    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->readers_queue.init();
    pipe->writers_queue.init();

    auto read_end = file_table.alloc();

    if (!read_end)
    {
        kfree(pipe);
        return read_end.error();
    }

    // we need to set the refs to 1, so that the next call to alloc
    // will not return the same file
    read_end->offset = 0;
    read_end->inode = nullptr;
    read_end->pipe = pipe;
    read_end->flags = O_RDONLY;
    read_end->refs = 1;
    read_end->ops = pipe_read_end_fops;
    pipe->read_end = read_end.ptr;

    auto write_end = file_table.alloc();

    if (!write_end)
    {
        // I'm not sure that we can call read_end->close() here
        read_end->refs = 0;
        kfree(pipe);
        return write_end.error();
    }

    write_end->offset = 0;
    write_end->inode = nullptr;
    write_end->pipe = pipe;
    write_end->flags = O_WRONLY;
    write_end->refs = 1;
    write_end->ops = pipe_write_end_fops;
    pipe->write_end = write_end.ptr;

    pipe->buffer = (u8*)vmm.alloc_pages(PIPE_PAGES, PE_WRITE);

    if (!pipe->buffer)
    {
        read_end->refs = 0;
        write_end->refs = 0;
        kfree(pipe);
        return -ERR_NO_MEM;
    }

    return pipe;
}

void Pipe::close()
{
    // we can run a C compiler but there is no way to free pages :/
    // vmm.free_pages(buffer, PIPE_PAGES);

    kfree(this);
}

int Pipe::fill_stat(stat* buf)
{
    buf->st_dev = 0;
    buf->st_ino = (u64)this;
    buf->st_nlink = 1;
    buf->st_mode = IT_FIFO | IP_RW;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_rdev = 0;
    buf->st_size = 0;
    buf->st_blksize = 4096;
    buf->st_blocks = 0;

    buf->st_atime = 0;
    buf->st_mtime = 0;
    buf->st_ctime = 0;

    buf->st_atimensec = 0;
    buf->st_mtimensec = 0;
    buf->st_ctimensec = 0;

    return 0;
}

isize pipe_read(File* file, void* buf, usize size, usize offset)
{
    Pipe* pipe = file->pipe;

    while (true)
    {
        // no writers and also the pipe is empty
        if (!pipe->write_end && pipe->data_size == 0)
            return 0;

        // writers are still alive, but the pipe is empty
        // so we sleep until a writer wakes us up
        if (pipe->data_size == 0)
        {
            wait_on(&pipe->readers_queue);
            continue;
        }

        // at this point we do the actual reading

        if (pipe->data_size < size)
            size = pipe->data_size;

        usize bytes_to_end = PIPE_SIZE - pipe->read_pos;

        if (size > bytes_to_end)
        {
            memcpy(buf, pipe->buffer + pipe->read_pos, bytes_to_end);
            memcpy((u8*)buf + bytes_to_end, pipe->buffer, size - bytes_to_end);
        }
        else
            memcpy(buf, pipe->buffer + pipe->read_pos, size);

        pipe->data_size -= size;
        pipe->read_pos = (pipe->read_pos + size) % PIPE_SIZE;
        pipe->writers_queue.wake_all();

        return size;
    }
}

isize pipe_write(File* file, const void* buf, usize size, usize offset)
{
    Pipe* pipe = file->pipe;

    while (true)
    {
        // no readers, so there is no point in writing
        // here we should send SIGPIPE, but we don't have that yet
        // so just kill the process
        if (!pipe->read_end)
        {
            running->exit(-2);
            return -2;
        }

        // readers are still alive, but the pipe is full
        // so sleep until a reader consumes some data
        if (pipe->data_size == PIPE_SIZE)
        {
            wait_on(&pipe->writers_queue);
            continue;
        }

        // at this point we do the actual writing

        if (PIPE_SIZE - pipe->data_size < size)
            size = PIPE_SIZE - pipe->data_size;

        usize bytes_to_end = PIPE_SIZE - pipe->write_pos;

        if (size > bytes_to_end)
        {
            memcpy(pipe->buffer + pipe->write_pos, buf, bytes_to_end);
            memcpy(pipe->buffer, (u8*)buf + bytes_to_end, size - bytes_to_end);
        }
        else
            memcpy(pipe->buffer + pipe->write_pos, buf, size);

        pipe->data_size += size;
        pipe->write_pos = (pipe->write_pos + size) % PIPE_SIZE;
        pipe->readers_queue.wake_all();

        return size;
    }
}

int pipe_read_end_close(File* file)
{
    Pipe* pipe = file->pipe;

    pipe->read_end = nullptr;
    pipe->writers_queue.wake_all();

    if (pipe->write_end == nullptr)
        pipe->close();

    return 0;
}

int pipe_write_end_close(File* file)
{
    Pipe* pipe = file->pipe;

    pipe->write_end = nullptr;
    pipe->readers_queue.wake_all();

    if (pipe->read_end == nullptr)
        pipe->close();

    return 0;
}

FileOps pipe_read_end_fops =
{
    .close = pipe_read_end_close,
    .read = pipe_read,
};

FileOps pipe_write_end_fops =
{
    .close = pipe_write_end_close,
    .write = pipe_write,
};
