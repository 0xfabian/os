#pragma once

#include <print.h>

enum ErrorCode
{
    ERR_SUCCESS = 0,
    ERR_NO_MEM,
    ERR_NO_EXEC,
    ERR_NO_DEV,
    ERR_NO_SPACE,
    ERR_NOT_FOUND,
    ERR_NOT_DIR,
    ERR_IS_DIR,
    ERR_DIR_NOT_EMPTY,
    ERR_BAD_ARG,
    ERR_BAD_FD,
    ERR_MNT_TABLE_FULL,
    ERR_INODE_TABLE_FULL,
    ERR_FILE_TABLE_FULL,
    ERR_FD_TABLE_FULL,
    ERR_EXISTS,
    ERR_FS_EXISTS,
    ERR_FS_BUSY,
    ERR_NOT_SAME_FS,
    ERR_MNT_EXISTS,
    ERR_MNT_BUSY,
    ERR_NOT_IMPL,
    ERR_MAX_CODE = 4095
};

template <typename T>
struct result_ptr
{
    union
    {
        T* ptr;
        intptr_t err;
    };

    result_ptr(T* ptr) : ptr(ptr) {}
    result_ptr(int err) : err(err) {}

    bool is_error() const
    {
        return (uintptr_t)ptr >= (uintptr_t)-ERR_MAX_CODE;
    }

    operator bool() const
    {
        return !is_error();
    }

    int error() const
    {
        return is_error() ? err : 0;
    }

    T* operator->()
    {
        if (is_error())
            panic("result_ptr dereference");

        return ptr;
    }

    T* or_nullptr()
    {
        return is_error() ? nullptr : ptr;
    }
};