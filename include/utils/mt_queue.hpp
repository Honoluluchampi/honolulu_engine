#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <condition_variable>

namespace hnll::utils {

// thread-safe queue on a double-linked list
template<typename T>
class mt_deque {
    // tail ----- next <- node -> prev ----- head
    struct node {
      u_ptr<T> data = nullptr;
      u_ptr<node> next = nullptr;
      node *prev = nullptr;
    };

  public:
    // assign a dummy node for fine-grained mutex lock
    mt_deque() : head_(std::make_unique<node>()), tail_(head_.get()) {}

    mt_deque(const mt_deque &) = delete;

    mt_deque &operator=(const mt_deque &) = delete;

    void push_tail(T &&new_value) {
      auto new_data = std::make_unique<T>(std::move(new_value));
      auto new_node = std::make_unique<node>();
      auto new_node_raw = new_node.get();
      {
        // assign data ptr
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        tail_->data = std::move(new_data);

        // set next-prev ptr
        tail_->next = std::move(new_node);
        new_node_raw->prev = tail_;
        tail_ = new_node_raw;
      }
      data_cond_.notify_one();
    }

    void push_front(T &&new_value) {
      // create new node with data
      auto new_node = std::make_unique<node>();
      new_node->data = std::make_unique<T>(std::move(new_value));

      {
        std::lock_guard<std::mutex> head_lock(head_mutex_);
        head_->prev = new_node.get();
        new_node->next = std::move(head_);
        head_ = std::move(new_node);
      }
      data_cond_.notify_one();
    }

    // in all pop functions, get head mutex first then get tail mutex
    // to avoid deadlock
    u_ptr<T> wait_pop_front() {
      // wait for data and lock head
      auto head_lock = wait_for_data();
      // lock tail
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);

      return pop_head();
    }

    u_ptr<T> wait_pop_back() {
      auto head_lock = wait_for_data();
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);
      return pop_tail();
    }

    u_ptr<T> try_pop_front() {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);

      // if empty
      if (head_.get() == tail_)
        return nullptr;
      return pop_head();
    }

    u_ptr<T> try_pop_back() {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);

      if (head_.get() == tail_)
        return nullptr;
      return pop_tail();
    }

    bool empty() {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      return (head_.get() == get_tail());
    }

  private:
    // get tail ptr thread-safely
    node *get_tail() {
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);
      return tail_;
    }

    std::unique_lock<std::mutex> wait_for_data() {
      std::unique_lock<std::mutex> head_lock(head_mutex_);
      // wait for at least one data is pushed to the queue
      data_cond_.wait(head_lock, [&] { return head_.get() != get_tail(); });
      return std::move(head_lock);
    }

    // the 2 mutexes should be taken by a caller
    u_ptr<T> pop_head() {
      auto old_head = std::move(head_);
      head_ = std::move(old_head->next);
      head_->prev = nullptr;

      return std::move(old_head->data);
    }

    // the 2 mutexes should be taken by a caller
    u_ptr<T> pop_tail() {
      tail_ = tail_->prev;
      tail_->next.reset();
      auto data = std::move(tail_->data);
      tail_->data = nullptr;

      return std::move(data);
    }

    std::mutex head_mutex_;
    u_ptr<node> head_;
    std::mutex tail_mutex_;
    node *tail_;
    std::condition_variable data_cond_;
};

} // namespace hnll::utils