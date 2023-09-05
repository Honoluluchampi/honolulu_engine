#pragma once

// hnll
#include <utils/common_alias.hpp>

// mt is synonym for "multi thread"

namespace hnll {

template <typename T>
class mt_queue
{
    struct node
    {
      u_ptr<T> data;
      u_ptr<node> next;
    };

  public:
    // assign a dummy node for fine-grained mutex lock
    mt_queue() : head_(std::make_unique<node>()), tail_(head_.get())
    {}

    mt_queue(const mt_queue&) = delete;
    mt_queue& operator=(const mt_queue&) = delete;

    // returns nullptr if the queue is empty
    u_ptr<T> try_pop()
    {
      auto old_head = pop_head();
      return old_head ? std::move(old_head->data) : nullptr;
    }

    void push(T&& new_value)
    {
      auto new_data = std::make_unique<T>(std::move(new_value));
      auto new_node = std::make_unique<node>();
      auto* const new_tail = new_node.get();
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);
      tail_->data = std::move(new_data);
      tail_->next = std::move(new_node);
      tail_ = new_tail;
    }

  private:
    // get tail ptr thread-safely
    node* get_tail()
    {
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);
      return tail_;
    }

    u_ptr<node> pop_head()
    {
      std::lock_guard<std::mutex> head_lock(head_mutex_);

      // if there is only the dummy node
      if (head_.get() == get_tail()) {
        return nullptr;
      }

      auto old_head = std::move(head_);
      head_ = std::move(old_head->next);

      return old_head;
    }

    std::mutex head_mutex_;
    u_ptr<node> head_;
    std::mutex tail_mutex_;
    node* tail_;
};

} // namespace hnll