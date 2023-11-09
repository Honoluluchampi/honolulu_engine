// hnll
#include <utils/thread_pool.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

constexpr int THREAD_COUNT = 4;
constexpr int ELEMENT_COUNT = 10000;
#define JOIN_THREADS(threads) for (auto& t : threads) t.join()

int task() { return 0; }
int task_with_args(float d, bool b) { return 1; }

TEST(thread_pool, single_task)
{
  utils::thread_pool tp;

  auto future_no_arg = tp.submit(task);
  auto future_args = tp.submit(task_with_args, 2.f, true);
  auto future_lambda = tp.submit([]() { return 2; });

  EXPECT_EQ(future_no_arg.get(), 0);
  EXPECT_EQ(future_args.get(), 1);
  EXPECT_EQ(future_lambda.get(), 2);
}

TEST(thread_pool, multiple_task)
{
  utils::thread_pool tp;

  auto begin = std::chrono::system_clock::now();

  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    // 100 ms
    tp.submit([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    // 1.2 sec
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  auto end = std::chrono::system_clock::now();

  EXPECT_TRUE(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() < 200);
}

int recursive_wait(utils::thread_pool& pool, int i) {
  if (i == 0)
    return 1;

  else {
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // add another task
//    auto future = pool.submit(recursive_wait, pool, i - 1);
//    return future.get() + 1;
    return recursive_wait(pool, i - 1) + 1;
  }
}

int recursive_wait2(int i) {
  if (i == 0)
    return 1;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return recursive_wait2(i - 1) + 1;
}

TEST(thread_pool, work_stealing)
{
  std::vector <std::future<int>> futures;
  utils::thread_pool pool;

  for (int i = 0; i < 12; i++) { //std::thread::hardware_concurrency(); i++) {
    futures.emplace_back(pool.submit(recursive_wait, pool, i));
  }

  int ans = 0;
  for (auto& future : futures) {
    ans += future.get();
  }

  EXPECT_EQ(ans, 78);

  // 6.6 sec
//  ans = 0;
//  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
//    ans += recursive_wait2(i);
//  }
//
//  EXPECT_EQ(ans, 78);
}

} // namespace hnll