#include <utils/thread_pool.hpp>

namespace hnll::utils {

thread_pool::thread_pool(int _thread_count) : done_(false), joiner_(threads_)
{
  // init worker threads
  const auto thread_count = _thread_count == 0 ?
    std::thread::hardware_concurrency() : _thread_count;
  try {
    for (int i = 0; i < thread_count; i++) {
      // create local queues and threads
      local_queues_.emplace_back(std::make_unique<mt_deque<function_wrapper>>());
      threads_.emplace_back(&thread_pool::worker_thread, this, i);
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

u_ptr<function_wrapper> thread_pool::try_pop_from_local_queue()
{ return local_queue_ ? local_queue_->try_pop_front() : nullptr; }

u_ptr<function_wrapper> thread_pool::try_pop_from_global_queue()
{ return global_queue_.try_pop_front(); }

u_ptr<function_wrapper> thread_pool::try_steal_task()
{
  u_ptr<function_wrapper> ret;

  for (unsigned i = 0; i < local_queues_.size() - 1; i++) {
    // not to look the first queue every time
    auto idx = (queue_index_ + i + 1) % local_queues_.size();

    if (ret = local_queues_[idx]->try_pop_back(); ret)
      return std::move(ret);
  }

  return nullptr;
}

} // namespace hnll::utils