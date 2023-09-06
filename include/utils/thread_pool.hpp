#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <condition_variable>

// mt is synonym for "multi thread"

namespace hnll {

// ------------------------ thread-safe queue on a single-linked list

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

    void push(T&& new_value)
    {
      auto new_data = std::make_unique<T>(std::move(new_value));
      auto new_node = std::make_unique<node>();
      {
        // assign data ptr
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        tail_->data = std::move(new_data);

        auto *const new_tail = new_node.get();
        tail_->next = std::move(new_node);
        tail_ = new_tail;
      }
      data_cond_.notify_one();
    }

    u_ptr<T> wait_and_pop()
    {
      auto old_head = wait_pop_head();
      return old_head->data; // TODO : move
    }

    // returns nullptr if the queue is empty
    u_ptr<T> try_pop()
    {
      auto old_head = try_pop_head();
      return old_head ? std::move(old_head->data) : nullptr;
    }

    bool empty()
    {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      return (head_.get() == get_tail());
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
      auto old_head = std::move(head_);
      head_ = std::move(old_head->next);

      return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data()
    {
      std::unique_lock<std::mutex> head_lock(head_mutex_);
      // wait for at least one data is pushed to the queue
      data_cond_.wait(head_lock, [&]{ return head_.get() != get_tail(); });
      return std::move(head_lock);
    }

    u_ptr<node> wait_pop_head()
    {
      auto head_lock = wait_for_data();
      return pop_head();
    }

    u_ptr<node> try_pop_head()
    {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      if (head_.get() == get_tail())
        return nullptr;
      return pop_head();
    }

    std::mutex head_mutex_;
    u_ptr<node> head_;
    std::mutex tail_mutex_;
    node* tail_;
    std::condition_variable data_cond_;
};

// ----------------------------------------------------------------------------

class threads_joiner
{
  public:
    explicit threads_joiner(std::vector<std::thread>& threads) : threads_(threads) {}

    ~threads_joiner()
    {
      for(auto& thread : threads_) {
        if (thread.joinable())
          thread.join();
      }
    }

  private:
    std::vector<std::thread>& threads_;
};

// ---------------------------------------------------------------------------

class thread_pool
{
  public:
    thread_pool() : done_(false), joiner_(threads_)
    {
      // init worker threads
      const auto thread_count = std::thread::hardware_concurrency();
      try {
        for (int i = 0; i < thread_count; i++) {
          threads_.emplace_back(std::thread(&thread_pool::worker_thread, this));
        }
      }
      catch (...) {
        // clean up all the threads before throwing an exception
        done_ = true;
        throw;
      }
    }

    ~thread_pool()
    {
      done_ = true;
      // joiner_ joins all the worker threads
    }

    // add task to the queue
    template <typename FunctionType>
    void submit(FunctionType f)
    { task_queue_.push(std::function<void()>(f)); }

  private:
    void worker_thread()
    {
      while (!done_) {
        auto task = task_queue_.try_pop();
        if (task != nullptr)
          (*task)(); // execute task
        else
          std::this_thread::yield();
      }
    }

    std::atomic_bool done_;
    mt_queue<std::function<void()>> task_queue_;
    std::vector<std::thread> threads_;
    threads_joiner joiner_;
};

} // namespace hnll