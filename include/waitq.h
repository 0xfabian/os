#pragma once

struct Task;

struct WaitQueue
{
    struct Node
    {
        Task* task;
        Node* next;
    };

    Node* head;
    Node* tail;

    void init();
    void add(Task* task);
    void remove(Task* task);
    void wake_all();
};

void wait_on(WaitQueue* queue);
