#pragma once

#include <print.h>
#include <error.h>
#include <task.h>

enum SyscallNumber
{
    SYS_DEBUG = 0
};

int syscall_handler(CPU* cpu, int num);

void sys_debug();