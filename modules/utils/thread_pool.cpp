#include <utils/thread_pool.hpp>

namespace hnll::utils {

thread_pool::thread_pool() : done_(false), joiner_(threads_)
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

thread_pool::~thread_pool()
{
  done_ = true;
  // joiner_ joins all the worker threads
}

void thread_pool::run_pending_task()
{
  if (auto task = try_pop_from_local_queue(); task)
    (*task)();
  else if (task = try_pop_from_global_queue(); task)
    (*task)();
  else if (task = try_steal_task(); task)
    (*task)();
  else
    std::this_thread::yield();
}

void thread_pool::worker_thread(unsigned int queue_index)
{
  queue_index_ = queue_index;
  // take local queue ptr
  assert(queue_index <= local_queues_.size() - 1);
  local_queue_ = local_queues_[queue_index_].get();

  while (!done_) {
    run_pending_task();
  }
}

} // namespace hnll::utils