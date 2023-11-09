// hnll
#include <utils/thread_pool.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

constexpr int THREAD_COUNT = 4;
constexpr int ELEMENT_COUNT = 10000;
#define JOIN_THREADS(threads) for (auto& t : threads) t.join()

int task() { return 1; }
int task_with_args(float d, bool b) { return 0; }

TEST(thread_pool, single_task)
{
  utils::thread_pool tp;

  std::this_thread::sleep_for(std::chrono::seconds(1));

  auto future = tp.submit(task);
  auto future2 = tp.submit(task_with_args, 2.f, true);

  std::packaged_task<int()> pt(task);
}

} // namespace hnll