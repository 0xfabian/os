#include <task.h>
#include <string.h>

Task* running;
Task* task_list;
Task* task_list_tail;

u64 global_tid = 0;

extern "C" void switch_now();

Task* alloc_task()
{
    Task* task = (Task*)kmalloc(sizeof(Task));
    task->mm = (MemoryMap*)kmalloc(sizeof(MemoryMap));

    task->next = nullptr;
    task->parent = running;
    task->tid = global_tid++;
    task->state = TASK_BORN;
    task->exit_code = 0;

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        task->fdt.files[i] = nullptr;

    return task;
}

Task* Task::from(void (*func)(void))
{
    Task* task = alloc_task();

    task->mm->pml4 = nullptr;
    task->mm->start = nullptr;
    task->mm->size = 0;
    task->mm->user_stack = nullptr;
    task->mm->kernel_stack = vmm.alloc_pages(KERNEL_STACK_PAGES, PE_WRITE);

    u64 kstack_top = (u64)task->mm->kernel_stack + KERNEL_STACK_SIZE;

    task->krsp = kstack_top - sizeof(CPU);

    CPU* cpu = (CPU*)task->krsp;
    memset(cpu, 0, sizeof(CPU));

    cpu->rbp = kstack_top;

    cpu->rip = (u64)func;
    cpu->cs = KERNEL_CS;
    cpu->rflags = 0x202;
    cpu->rsp = kstack_top;
    cpu->ss = KERNEL_DS;

    return task;
}

int load_elf(Task* task, const char* path, u64* entry)
{
    auto src = File::open(path, 0);

    if (!src)
        return src.error();

    ELF::Header hdr;

    isize read = src->pread((char*)&hdr, sizeof(ELF::Header), 0);

    if (read != sizeof(ELF::Header) || !hdr.supported())
    {
        ikprintf(WARN "Invalid ELF header\n");
        src->close();
        return -ERR_NO_EXEC;
    }

    ELF::ProgramHeader phdrs[hdr.phnum];

    read = src->pread((char*)phdrs, hdr.phnum * hdr.phentsize, hdr.phoff);

    if (read != hdr.phnum * hdr.phentsize)
    {
        ikprintf(WARN "Invalid program headers\n");
        src->close();
        return -ERR_NO_EXEC;
    }

    task->mm->pml4 = vmm.make_user_page_table();
    task->mm->start = nullptr;
    task->mm->size = 0;
    task->mm->user_stack = vmm.alloc_pages(task->mm->pml4, USER_STACK_BOTTOM, USER_STACK_PAGES, PE_WRITE | PE_USER);
    // we don't touch the task's kernel stack

    vmm.switch_pml4(task->mm->pml4);

    for (ELF::ProgramHeader* phdr = phdrs; phdr < phdrs + hdr.phnum; phdr++)
    {
        // skip non-loadable segments
        if (phdr->type != 1)
            continue;

        // is this possible anyway?
        if (phdr->align != PAGE_SIZE)
            ikprintf(WARN "Non-page aligned segment\n");

        void* addr = vmm.alloc_pages(task->mm->pml4, phdr->vaddr, PAGE_COUNT(phdr->memsz), PE_WRITE | PE_USER);

        src->pread((char*)addr, phdr->filesz, phdr->offset);
        memset((u8*)addr + phdr->filesz, 0, phdr->memsz - phdr->filesz);
    }

    src->close();

    *entry = hdr.entry;

    return 0;
}

Task* Task::from(const char* path)
{
    Task* task = alloc_task();

    task->mm->kernel_stack = vmm.alloc_pages(KERNEL_STACK_PAGES, PE_WRITE);

    u64 entry;
    if (load_elf(task, path, &entry) != 0)
        return nullptr;

    u64 ustack_top = (u64)task->mm->user_stack + USER_STACK_SIZE;
    u64 kstack_top = (u64)task->mm->kernel_stack + KERNEL_STACK_SIZE;

    task->krsp = kstack_top - sizeof(CPU);

    CPU* cpu = (CPU*)task->krsp;
    memset(cpu, 0, sizeof(CPU));

    cpu->rbp = ustack_top;

    cpu->rip = entry;
    cpu->cs = USER_CS;
    cpu->rflags = 0x202;
    cpu->rsp = ustack_top;
    cpu->ss = USER_DS;

    cpu->rsp -= 8;
    *(u64*)cpu->rsp = 0; // the null terminator

    cpu->rsp -= 8;
    *(u64*)cpu->rsp = 0; // argc

    // since Task::from can't be called from a user task
    // this is probably running under a kernel task
    // so no need for switching the page table

    // if (running->mm->pml4)
    //     vmm.switch_pml4(running->mm->pml4);

    return task;
}

Task* Task::dummy()
{
    // this should be the first task in the queue
    // at the first switch, the kmain will store the cpu state
    // to this dummy task so basically this task is kmain

    Task* task = alloc_task();

    task->mm->pml4 = nullptr;
    task->mm->start = nullptr;
    task->mm->size = 0;
    task->mm->user_stack = nullptr;
    task->mm->kernel_stack = nullptr;

    // don't need to initialize krsp or the whole CPU struct

    return task;
}

Task* Task::fork()
{
    Task* task = alloc_task();

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        if (running->fdt.files[i])
            task->fdt.files[i] = running->fdt.files[i]->dup();

    task->mm->pml4 = vmm.make_user_page_table();
    task->mm->start = running->mm->start;
    task->mm->size = running->mm->size;
    task->mm->user_stack = running->mm->user_stack;
    task->mm->kernel_stack = vmm.alloc_pages(KERNEL_STACK_PAGES, PE_WRITE);

    // now here we should copy the mappings
    // and also copy the memory itself
    // no fancy copy-on-write for now

    PML4* src_pml4 = running->mm->pml4;

    for (u64 i = 0; i < 256; i++)
    {
        if (!src_pml4->has(i))
            continue;

        PDPT* pdpt = (PDPT*)(src_pml4->get(i) | KERNEL_HHDM);

        for (u64 j = 0; j < 512; j++)
        {
            if (!pdpt->has(j))
                continue;

            PD* pd = (PD*)(pdpt->get(j) | KERNEL_HHDM);

            for (u64 k = 0; k < 512; k++)
            {
                if (!pd->has(k))
                    continue;

                PT* pt = (PT*)(pd->get(k) | KERNEL_HHDM);

                for (u64 l = 0; l < 512; l++)
                {
                    if (!pt->has(l))
                        continue;

                    u64 virt = i << 39 | j << 30 | k << 21 | l << 12;
                    u64 flags = pt->entries[l] & 0xfff;

                    // this is a bit tricky
                    // we need to copy running's memory to the new task
                    // but we can't just copy the memory
                    // because the memory mapping is the same
                    // so we need to copy it interally at the kernel address
                    // of the corresponding page

                    vmm.alloc_page_and_copy(task->mm->pml4, virt, flags);
                }
            }
        }
    }

    // now only the kernel stack changes
    u64 kstack_top = (u64)task->mm->kernel_stack + KERNEL_STACK_SIZE;

    task->krsp = kstack_top - sizeof(CPU);

    CPU* cpu = (CPU*)task->krsp;
    memcpy(cpu, (CPU*)running->krsp, sizeof(CPU));

    cpu->rax = 0;

    return task;
}

int Task::execve(const char* path, char* const argv[], char* const envp[])
{
    // we save the argv in kernel memory
    // because we will change the page table

    usize argv_size = 0;
    usize argc = 0;

    for (usize i = 0; argv[i]; i++)
    {
        argv_size += strlen(argv[i]) + 1;
        argc++;
    }

    u8* argv_mem = (u8*)kmalloc(argv_size);

    usize off = 0;

    for (usize i = 0; argv[i]; i++)
    {
        usize len = strlen(argv[i]) + 1;
        memcpy(argv_mem + off, argv[i], len);
        off += len;
    }

    // loading the elf will replace the page table
    // but it will keep mm->kernel_stack
    u64 entry;
    if (load_elf(this, path, &entry) != 0)
    {
        kfree(argv_mem);
        return -1;
    }

    // !!! we are not freeing the previous mm

    u64 ustack_top = (u64)mm->user_stack + USER_STACK_SIZE;

    CPU* cpu = (CPU*)krsp;
    memset(cpu, 0, sizeof(CPU));

    cpu->rbp = ustack_top;
    cpu->rip = entry;
    cpu->cs = USER_CS;
    cpu->rflags = 0x202;
    cpu->rsp = ustack_top;
    cpu->ss = USER_DS;
    cpu->rcx = cpu->rip;
    cpu->r11 = cpu->rflags;

    // i don't know if it's a good idea to store
    // the actual argv strings on the stack too

    cpu->rsp -= argv_size;
    memcpy((void*)cpu->rsp, argv_mem, argv_size);

    u64 stack_addr = cpu->rsp;

    // we should align the stack to 16 bytes
    cpu->rsp &= ~0x0f;

    cpu->rsp -= 8;
    *(u64*)cpu->rsp = 0; // the null terminator

    off = 0;

    cpu->rsp -= 8 * argc;
    for (usize i = 0; i < argc; i++)
    {
        *(u64*)(cpu->rsp + i * 8) = (u64)stack_addr + off;
        off += strlen((const char*)(stack_addr + off)) + 1;
    }

    kfree(argv_mem);

    cpu->rsp -= 8;
    *(u64*)cpu->rsp = argc;

    // very important since we store the user stack here
    tss.rsp2 = cpu->rsp;

    return 0;
}

void Task::exit(int code)
{
    state = TASK_ZOMBIE;
    exit_code = code;

    if (parent->state == TASK_SLEEPING)
        parent->return_from_syscall(running->tid);

    schedule();
    switch_now();
}

void Task::wait()
{
    // hacky way to do wait

    int children = 0;

    Task* task = task_list;

    do
    {
        if (task->state != TASK_ZOMBIE && task->parent == running)
            children++;

        task = task->next;

    } while (task != task_list);

    if (children)
        running->sleep();
}

void Task::ready()
{
    if (task_list)
    {
        task_list_tail->next = this;
        task_list_tail = this;
        task_list_tail->next = task_list;
    }
    else
    {
        task_list = this;
        task_list->next = task_list;
        task_list_tail = task_list;
    }

    state = TASK_READY;
}

void Task::sleep()
{
    state = TASK_SLEEPING;

    // kmain is always ready
    schedule();
    switch_now();
}

void Task::return_from_syscall(int ret)
{
    CPU* cpu = (CPU*)krsp;
    cpu->rax = ret;

    state = TASK_READY;
}

void sched_init()
{
    kprintf(INFO "Initializing scheduler...\n");

    Task* t0 = Task::dummy();

    t0->ready();

    running = task_list;

    pic::set_irq(0, false);
    pic::set_irq(1, false);

    sti();
}

Task* get_next_task()
{
    Task* task = running->next;

    while (task->state != TASK_READY)
        task = task->next;

    return task;
}

void schedule()
{
    Task* next = get_next_task();

    if (next == running)
        return;

    running = next;

    if (running->mm->user_stack)
        tss.rsp0 = (u64)running->mm->kernel_stack + KERNEL_STACK_SIZE;

    if (running->mm->pml4)
        vmm.switch_pml4(running->mm->pml4);
}