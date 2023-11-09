// hnll
#include <utils/thread_pool.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

constexpr int THREAD_COUNT = 4;
constexpr int ELEMENT_COUNT = 10000;
#define JOIN_THREADS(threads) for (auto& t : threads) t.join()

int task() { return 1; }

TEST(thread_pool, single_task)
{
  utils::thread_pool tp;

  auto future_int = tp.submit_int(task);
  auto future = tp.submit(task);

  std::packaged_task<int()> pt(task);
}

} // namespace hnll