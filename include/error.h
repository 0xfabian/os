#pragma once

#include <print.h>

enum ErrorCode
{
    ERR_SUCCESS = 0,
    ERR_NOT_FOUND,
    ERR_NOT_DIR,
    ERR_BAD_FD,
    ERR_FD_TABLE_FULL,
    ERR_MNT_EXISTS,
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
};