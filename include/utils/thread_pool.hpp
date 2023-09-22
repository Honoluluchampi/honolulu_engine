#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <utils/mt_queue.hpp>

// std
#include <future>

// mt is synonym for "multi thread"

namespace hnll::utils {

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
    thread_pool();

    ~thread_pool();

    // add task to the queue
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f);

    void run_pending_task();

  private:
    void worker_thread(unsigned queue_index);

    u_ptr<function_wrapper> try_pop_from_local_queue();
    u_ptr<function_wrapper> try_pop_from_global_queue();
    u_ptr<function_wrapper> try_steal_task();

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

template <typename FunctionType>
std::future<typename std::result_of<FunctionType()>::type> thread_pool::submit(FunctionType f)
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

} // namespace hnll::utils