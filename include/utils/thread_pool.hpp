#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <condition_variable>
#include <future>
#include <queue>

// mt is synonym for "multi thread"

namespace hnll {

// ------------------------ thread-safe queue on a double-linked list

template <typename T>
class mt_deque
{
    // tail ----- next <- node -> prev ----- head
    struct node
    {
      u_ptr<T> data = nullptr;
      u_ptr<node> next = nullptr;
      node* prev = nullptr;
    };

  public:
    // assign a dummy node for fine-grained mutex lock
    mt_deque() : head_(std::make_unique<node>()), tail_(head_.get())
    {}

    mt_deque(const mt_deque&) = delete;
    mt_deque& operator=(const mt_deque&) = delete;

    void push_tail(T&& new_value)
    {
      auto new_data = std::make_unique<T>(std::move(new_value));
      auto new_node = std::make_unique<node>();
      {
        // assign data ptr
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        tail_->data = std::move(new_data);

        // set next-prev ptr
        tail_->next = std::move(new_node);
        new_node->prev = tail_;

        tail_ = new_node.get();
      }
      data_cond_.notify_one();
    }

    void push_front(T&& new_value)
    {
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
    u_ptr<T> wait_pop_front()
    {
      // wait for data and lock head
      auto head_lock = wait_for_data();
      // lock tail
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);

      return pop_head();
    }

    u_ptr<T> wait_pop_back()
    {
      auto head_lock = wait_for_data();
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);
      return pop_tail();
    }

    u_ptr<T> try_pop_front()
    {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);

      // if empty
      if (head_.get() == tail_)
        return nullptr;
      return pop_head();
    }

    u_ptr<T> try_pop_back()
    {
      std::lock_guard<std::mutex> head_lock(head_mutex_);
      std::lock_guard<std::mutex> tail_lock(tail_mutex_);

      if (head_.get() == tail_)
        return nullptr;
      return pop_tail();
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

    std::unique_lock<std::mutex> wait_for_data()
    {
      std::unique_lock<std::mutex> head_lock(head_mutex_);
      // wait for at least one data is pushed to the queue
      data_cond_.wait(head_lock, [&]{ return head_.get() != get_tail(); });
      return std::move(head_lock);
    }

    // the 2 mutexes should be taken by a caller
    u_ptr<T> pop_head()
    {
      auto old_head = std::move(head_);
      head_ = std::move(old_head->next);
      head_->prev = nullptr;

      return std::move(old_head->data);
    }

    // the 2 mutexes should be taken by a caller
    u_ptr<T> pop_tail()
    {
      tail_ = tail_->prev;
      tail_->next.reset();
      auto data = std::move(tail_->data);
      tail_->data = nullptr;

      return std::move(data);
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

class function_wrapper
{
    struct impl_base
    {
      virtual void call() = 0;
      virtual ~impl_base() {}
    };

  public:
    template <typename F>
    explicit function_wrapper(F&& f) : impl_(new impl_type<F>(std::move(f))) {}

    void operator()() { impl_->call(); }
    function_wrapper() = default;
    function_wrapper(function_wrapper&& other) : impl_(std::move(other.impl_)) {}
    function_wrapper& operator=(function_wrapper&& other)
    {
      impl_ = std::move(other.impl_);
      return *this;
    }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&) = delete;

  private:
    template <typename F>
    struct impl_type : impl_base
    {
      F f;
      explicit impl_type(F&& f_) : f(std::move(f_)) {}
      void call() override { f(); }
    };

    u_ptr<impl_base> impl_;
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
          // create local queues and threads
          local_queues_.emplace_back(std::make_unique<mt_deque<function_wrapper>>());
          threads_.emplace_back(std::thread(&thread_pool::worker_thread, this, i));
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
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f)
    {
      // infer result type
      typedef typename std::result_of<FunctionType()>::type result_type;
      // init task with future
      std::packaged_task<result_type()> task(std::move(f));
      // preserve the future before moving this to the queue
      auto task_future = task.get_future();

      if (local_queue_) {
        local_queue_->push_front(std::move(task));
      }
      // main thread doesn't have local queue
      else {
        global_queue_.push_tail(std::move(task));
      }
      return task_future;
    }

    void run_pending_task()
    {
      if (auto task = pop_task_from_local_queue(); task)
        (*task)();
      else if (task = pop_task_from_global_queue(); task)
        (*task)();
      else if (task = steal_task(); task)
        (*task)();
      else
        std::this_thread::yield();
    }

  private:
    void worker_thread(unsigned queue_index)
    {
      queue_index_ = queue_index;
      // take local queue ptr
      assert(queue_index <= local_queues_.size() - 1);
      local_queue_ = local_queues_[queue_index_].get();

      while (!done_) {
        run_pending_task();
      }
    }

    u_ptr<function_wrapper> pop_task_from_local_queue()
    {
      return local_queue_ ? local_queue_->try_pop_front() : nullptr;
    }

    u_ptr<function_wrapper> pop_task_from_global_queue()
    {
      return global_queue_.try_pop_front();
    }

    u_ptr<function_wrapper> steal_task()
    {
      u_ptr<function_wrapper> ret;

      for (unsigned i = 0; i < local_queues_.size(); i++) {
        // myself
        if (local_queues_[i].get() == local_queue_)
          continue;

        if (ret = local_queues_[i]->try_pop_back(); ret)
          return std::move(ret);
      }

      return nullptr;
    }

    std::atomic_bool done_;

    // each thread takes a task only if it's local queue is empty
    mt_deque<function_wrapper> global_queue_;
    // local queues and threads is initialized in worker_thread()
    std::vector<u_ptr<mt_deque<function_wrapper>>> local_queues_;
    std::vector<std::thread> threads_;
    threads_joiner joiner_;

    static thread_local mt_deque<function_wrapper>* local_queue_;
    static thread_local unsigned queue_index_;
};

} // namespace hnll