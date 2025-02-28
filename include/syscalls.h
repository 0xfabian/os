#pragma once

#include <print.h>
#include <error.h>
#include <task.h>

enum SyscallNumber
{
    SYS_DEBUG = 0
};

void sys_debug();