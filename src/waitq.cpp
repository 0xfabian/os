#include <waitq.h>
#include <task.h>

void WaitQueue::init()
{
    head = nullptr;
    tail = nullptr;
}

void WaitQueue::add(Task* task)
{
    Node* node = (Node*)kmalloc(sizeof(Node));

    node->task = task;
    node->next = nullptr;

    if (!head)
        head = node;
    else
        tail->next = node;

    tail = node;
}

void WaitQueue::wake_all()
{
    while (head)
    {
        Node* node = head;
        node->task->ready();
        kfree(node);

        head = head->next;
    }

    tail = nullptr;
}

void wait_on(WaitQueue* queue)
{
    queue->add(running);
    pause();
}