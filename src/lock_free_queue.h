#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <atomic>
#include <iostream>
#include <memory>

template <typename T>
class LockFreeQueue {
   private:
    struct node {
        std::shared_ptr<T> data;
        std::atomic<node*> next;
        node(T const& data_) : data(std::make_shared<T>(data_)) {}
    };
    std::atomic<node*> head;
    std::atomic<node*> tail;

   public:
    void Push(T const& data) {
        /*
        head -> ...->...-> tail
        */
        std::atomic<node*> const new_node = new node(data);
        node* old_tail = tail.load();
        while (old_tail && old_tail->next && !std::atomic_compare_exchange_weak(&old_tail->next, nullptr, new_node)) {
            node* old_tail = tail.load();
        }
        tail.compare_exchange_weak(old_tail, new_node);

        /*
        The first parameter of atomic<T>::compare_exhange_strong is a T&. It requires an lvalue. That's one half of the
        "exchange": the current value of the atomic is loaded into the object referred to by the first parameter.
        */
        node* tmp = nullptr;
        head.compare_exchange_weak(tmp, new_node);
    }
    std::shared_ptr<T> Pop() {
        node* old_head = head.load();
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next)) {
            old_head = head.load();
        }
        return old_head ? old_head->data : std::shared_ptr<T>();
    }
};

#endif