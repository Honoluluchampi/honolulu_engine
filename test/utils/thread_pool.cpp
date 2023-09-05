// hnll
#include <utils/thread_pool.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

struct hoge
{
  std::vector<int> data;
};

TEST(thread_pool, mt_queue)
{
  std::vector<int> input = { 1, 7, 2, 3, 7, 5, 0, 4 };

  mt_queue<int> queue;

  for (auto& val : input) {
    queue.push(std::move(val));
  }

  for (int i = 0; i < input.size(); i++) {
    EXPECT_EQ(input[i], *queue.try_pop());
  }
}

} // namespace hnll