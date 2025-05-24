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

    task->waitq = this;
}

void WaitQueue::remove(Task* task)
{
    Node* prev = nullptr;
    Node* curr = head;

    while (curr)
    {
        if (curr->task == task)
        {
            task->waitq = nullptr;

            if (prev)
                prev->next = curr->next;
            else
                head = curr->next;

            if (curr == tail)
                tail = prev;

            kfree(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

void WaitQueue::wake_all()
{
    while (head)
    {
        Node* node = head;
        node->task->waitq = nullptr;
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