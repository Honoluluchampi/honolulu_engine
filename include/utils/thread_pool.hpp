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

    // head_mutex_ should be taken by a caller
    u_ptr<node> pop_head()
    {
      auto old_head = std::move(head_);
      head_ = std::move(old_head->next);
      head_->prev = nullptr;

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
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f)
    {
      // infer result type
      typedef typename std::result_of<FunctionType()>::type result_type;
      // init task with future
      std::packaged_task<result_type()> task(std::move(f));
      // preserve the future before moving this to the queue
      auto task_future = task.get_future();

      if (local_queue_) {
        local_queue_->push(std::move(task));
      }
      // main thread doesn't have local queue
      else {
        global_queue_.push_tail(std::move(task));
      }
      return task_future;
    }

    void run_pending_task()
    {
      // if this thread has local queue and it is not empty
      if (local_queue_ && !local_queue_->empty()) {
        auto task = std::move(local_queue_->front());
        local_queue_->pop();
        task();
      }
      else if (auto task = global_queue_.try_pop(); task) {
        (*task)();
      }
      else {
        std::this_thread::yield();
      }
    }

  private:
    void worker_thread()
    {
      // init local queue
      local_queue_ = std::make_unique<local_queue>();

      while (!done_) {
        run_pending_task();
      }
    }

    std::atomic_bool done_;

    // each thread takes a task only if it's local queue is empty
    mt_deque<function_wrapper> global_queue_;
    // local thread is initialized in worker_thread()
    // this queue is destructed when the thread exits
    using local_queue = std::queue<function_wrapper>;
    static thread_local u_ptr<local_queue> local_queue_;

    std::vector<std::thread> threads_;
    threads_joiner joiner_;
};

} // namespace hnll