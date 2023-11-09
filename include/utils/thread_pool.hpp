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

    template <typename F>
    struct impl_type : impl_base
    {
      F f;
      explicit impl_type(F&& f_) : f(std::move(f_)) {}
      void call() override { f(); }
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
    u_ptr<impl_base> impl_;
};

// ---------------------------------------------------------------------------

class thread_pool
{
  public:
    thread_pool(int thread_count = 0);
    ~thread_pool();

    // add task to the queue by perfect forwarding arguments
    template <typename Func, typename... Args, typename Result = std::invoke_result_t<Func, Args...>>
    std::future<Result> submit(Func&& f, Args&&... args);

    void run_pending_task();
    void wait_for_all_tasks();

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

template <typename Func, typename... Args, typename Result>
std::future<Result> thread_pool::submit(Func&& f, Args&&... args)
{
  // init task with future
  auto task = std::packaged_task<Result()>(
    [f = std::tuple<Func>(std::forward<Func>(f)), args = std::tuple<Args...>(std::forward<Args>(args)...)]()
    { return std::apply(std::get<0>(f), args); }
  );

  // preserve the future before moving this to the queue
  auto task_future = task.get_future();
  auto task_wrapper = function_wrapper{ std::move(task) };

  if (local_queue_) {
    local_queue_->push_front(std::move(task_wrapper));
  }
  // main thread doesn't have local queue
  else {
    global_queue_.push_tail(std::move(task_wrapper));
  }
  return task_future;
}

} // namespace hnll::utils