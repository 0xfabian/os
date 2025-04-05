#include <task.h>
#include <string.h>

u64 global_tid = 0;

Task* running = nullptr;
Task* idle_task = nullptr;
Task* rq_head = nullptr;

extern "C" void switch_now();

Task* alloc_task()
{
    Task* task = (Task*)kmalloc(sizeof(Task));

    task->parent = running;
    task->active_children = 0;
    task->tid = global_tid++;

    task->state = TASK_BORN;
    task->mm = (MemoryMap*)kmalloc(sizeof(MemoryMap));
    task->cwd_str = strdup("/");
    task->cwd = root_mount->sb->root->get();

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        task->fdt.files[i] = nullptr;

    task->exit_code = 0;

    task->next = nullptr;
    task->prev = nullptr;

    return task;
}

Task* Task::from(void (*func)(void))
{
    Task* task = alloc_task();

    task->mm->pml4 = nullptr;
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

    if (!src->inode->is_reg() || !src->inode->is_executable())
    {
        src->close();
        return -ERR_NO_EXEC;
    }

    ELF::Header hdr;

    isize read = src->pread((char*)&hdr, sizeof(ELF::Header), 0);

    if (read != sizeof(ELF::Header) || !hdr.supported())
    {
        kprintf(WARN "Invalid ELF header\n");
        src->close();
        return -ERR_NO_EXEC;
    }

    ELF::ProgramHeader phdrs[hdr.phnum];

    read = src->pread((char*)phdrs, hdr.phnum * hdr.phentsize, hdr.phoff);

    if (read != hdr.phnum * hdr.phentsize)
    {
        kprintf(WARN "Invalid program headers\n");
        src->close();
        return -ERR_NO_EXEC;
    }

    task->mm->pml4 = vmm.make_user_page_table();
    task->mm->user_stack = vmm.alloc_pages(task->mm->pml4, USER_STACK_BOTTOM, USER_STACK_PAGES, PE_WRITE | PE_USER);
    // we don't touch the task's kernel stack

    vmm.switch_pml4(task->mm->pml4);

    // kprintf("executable contains %d segments\n", hdr.phnum);

    u64 elf_end = 0;

    for (ELF::ProgramHeader* phdr = phdrs; phdr < phdrs + hdr.phnum; phdr++)
    {
        // kprintf("segment align: %lx filesize: %lx flags: %x memsz: %lx offset: %lx paddr: %lx type: %x vaddr: %lx\n", phdr->align, phdr->filesz, phdr->flags, phdr->memsz, phdr->offset, phdr->paddr, phdr->type, phdr->vaddr);

        // skip non-loadable segments
        if (phdr->type != PT_LOAD)
            continue;

        // // is this possible anyway? YES
        // if (phdr->align != PAGE_SIZE)
        //     kprintf(WARN "Non-page aligned segment %lx\n", phdr->align);

        u64 aligned_vaddr = phdr->vaddr & ~0xfff;
        u64 gap = phdr->vaddr & 0xfff;
        u64 pages = PAGE_COUNT(phdr->memsz + gap);

        vmm.alloc_pages(task->mm->pml4, aligned_vaddr, pages, PE_WRITE | PE_USER);

        memset((u8*)aligned_vaddr, 0, gap);
        src->pread((u8*)phdr->vaddr, phdr->filesz, phdr->offset);
        memset((u8*)phdr->vaddr + phdr->filesz, 0, phdr->memsz - phdr->filesz);

        u64 new_end = phdr->vaddr + phdr->memsz;

        if (new_end > elf_end)
            elf_end = new_end;
    }

    src->close();

    task->mm->start_brk = PAGE_ALIGN(elf_end);
    task->mm->brk = task->mm->start_brk;

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
    // this is running under a kernel task
    // so no need for switching the page table

    return task;
}

Task* Task::dummy()
{
    // this should be the first task in the queue
    // at the first switch, the kmain will store the cpu state
    // to this dummy task so basically this task is kmain

    Task* task = alloc_task();

    task->mm->pml4 = nullptr;
    task->mm->user_stack = nullptr;
    task->mm->kernel_stack = nullptr;

    // don't need to initialize krsp or the whole CPU struct

    return task;
}

Task* Task::fork()
{
    Task* task = alloc_task();

    kfree(task->cwd_str);
    task->cwd->put();

    task->cwd_str = strdup(running->cwd_str);
    task->cwd = running->cwd->get();

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        if (running->fdt.files[i])
            task->fdt.files[i] = running->fdt.files[i]->dup();

    task->mm->pml4 = vmm.make_user_page_table();
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

#define AT_NULL         0
#define AT_EXECFN       31
#define AT_PLATFORM     15
#define AT_RANDOM       25

struct auxv
{
    u64 type;
    u64 value;
};

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
    int err = load_elf(this, path, &entry);

    if (err)
    {
        kfree(argv_mem);
        return err;
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

    cpu->rsp -= 16;
    char* filename = (char*)cpu->rsp;
    strcpy(filename, "/mnt/bin/tcc");

    cpu->rsp -= 16;
    char* platform = (char*)cpu->rsp;
    strcpy(platform, "x86_64");

    cpu->rsp -= 16;
    void* random = (void*)cpu->rsp;
    memset(random, 0, 16);

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { AT_NULL, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { AT_PLATFORM, (u64)platform };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { AT_EXECFN, (u64)filename };

    // cpu->rsp -= sizeof(auxv);
    // *(auxv*)cpu->rsp = { 0x1a, HW_CAP2}

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { AT_RANDOM, (u64)random };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x17, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0xe, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0xd, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0xc, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0xb, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x9, 0x401ef0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x8, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x7, 0 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x5, 10 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x4, sizeof(ELF::ProgramHeader) };

    // cpu->rsp -= sizeof(auxv);
    // *(auxv*)cpu->rsp = { 0x3, ephdr };

    // cpu->rsp -= sizeof(auxv);
    // *(auxv*)cpu->rsp = { 0x11, 64 };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x6, PAGE_SIZE };

    // cpu->rsp -= sizeof(auxv);
    // *(auxv*)cpu->rsp = { 0x10, HW_CAP };

    cpu->rsp -= sizeof(auxv);
    *(auxv*)cpu->rsp = { 0x33, 0xd30 };

    // cpu->rsp -= sizeof(auxv);
    // *(auxv*)cpu->rsp = { 0x21, ? ? ? };

    cpu->rsp -= 8;
    *(u64*)cpu->rsp = 0; // envp null terminator, basically no env

    cpu->rsp -= 8;
    *(u64*)cpu->rsp = 0; // argv null terminator

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
    if (this == running)
        tss.rsp2 = cpu->rsp;

    return 0;
}

void Task::enter_rq()
{
    if (!rq_head)
    {
        rq_head = this;
        next = this;
        prev = this;
    }
    else
    {
        next = rq_head;
        prev = rq_head->prev;
        rq_head->prev->next = this;
        rq_head->prev = this;
    }
}

void Task::leave_rq()
{
    // more then one task in the ready queue
    if (rq_head != rq_head->next)
    {
        if (this == rq_head)
            rq_head = next;

        prev->next = next;
        next->prev = prev;
    }
    else
        rq_head = nullptr;

    next = nullptr;
    prev = nullptr;
}

void Task::ready()
{
    state = TASK_READY;

    enter_rq();

    if (parent)
        parent->active_children++;
}

void Task::sleep()
{
    leave_rq();

    state = TASK_SLEEPING;

    schedule();
}

void Task::exit(int code)
{
    leave_rq();

    state = TASK_ZOMBIE;
    exit_code = code;

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        if (fdt.files[i])
            fdt.files[i]->close();

    if (parent)
    {
        parent->active_children--;

        if (parent->state == TASK_SLEEPING)
            parent->return_from_syscall(running->tid);
    }

    // this is temporary
    // this should be freed in the TASK_DEAD state
    // after the parent has read the exit code

    kfree(cwd_str);
    cwd->put();

    // this doesn't properly free the memory
    // we should also free the pages allocated in the page table
    kfree(mm);
    kfree(this);

    schedule();
}

void Task::wait()
{
    if (active_children > 0)
        sleep();
}

void Task::return_from_syscall(int ret)
{
    CPU* cpu = (CPU*)krsp;
    cpu->rax = ret;

    ready();
}

void sched_init()
{
    kprintf(INFO "Initializing scheduler...\n");

    pit::init(100); // context switch every 10ms

    // the idle task is special
    // it executes only when the rq is empty
    idle_task = Task::from(idle);
    idle_task->tid = -1;
    idle_task->state = TASK_IDLE;

    global_tid = 0;

    Task* kmain = Task::dummy();

    kmain->ready();

    running = kmain;

    pic::unmask_irq(0);
    pic::unmask_irq(1);

    sti();
}

Task* pick_next()
{
    if (running->state == TASK_READY)
        return running->next;

    if (rq_head)
        return rq_head;

    return idle_task;
}

void schedule()
{
    Task* next = pick_next();

    if (next != running)
    {
        if (next->mm->user_stack)
            tss.rsp0 = (u64)next->mm->kernel_stack + KERNEL_STACK_SIZE;

        if (next->mm->pml4)
            vmm.switch_pml4(next->mm->pml4);

        running = next;
    }

    switch_now();
}

void leave_and_sched()
{
    running->sleep();
}