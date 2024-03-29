// hnll
#include <utils/mt_queue.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

constexpr int THREAD_COUNT = 4;
constexpr int ELEMENT_COUNT = 10000;
#define JOIN_THREADS(threads) for (auto& t : threads) t.join()

TEST(mt_queue, push_tail_pop_front)
{
  std::vector<int> input = { 1, 7, 2, 3, 7, 5, 0, 4 };
  utils::mt_deque<int> queue;

  // push values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue]() {
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          int k = j;
          queue.push_tail(std::move(k));
        }
      });
    }

    JOIN_THREADS(threads);
  }

  // pop values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    std::vector<int> anss(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue, &anss, i]() {
        int ans = 0;
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          ans += *queue.try_pop_front();
        }
        anss[i] = ans;
      });
    }

    JOIN_THREADS(threads);

    int sum = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
      sum += anss[i];
    }

    EXPECT_EQ(sum, (ELEMENT_COUNT - 1) * ELEMENT_COUNT / 2 * THREAD_COUNT);
  }
}

TEST(mt_queue, push_front_pop_front)
{
  std::vector<int> input = { 1, 7, 2, 3, 7, 5, 0, 4 };
  utils::mt_deque<int> queue;

  // push values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue]() {
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          int k = j;
          queue.push_front(std::move(k));
        }
      });
    }

    JOIN_THREADS(threads);
  }

  // pop values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    std::vector<int> anss(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue, &anss, i]() {
        int ans = 0;
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          ans += *queue.try_pop_front();
        }
        anss[i] = ans;
      });
    }

    JOIN_THREADS(threads);

    int sum = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
      sum += anss[i];
    }

    EXPECT_EQ(sum, (ELEMENT_COUNT - 1) * ELEMENT_COUNT / 2 * THREAD_COUNT);
  }
}

TEST(mt_queue, push_tail_pop_tail)
{
  std::vector<int> input = { 1, 7, 2, 3, 7, 5, 0, 4 };
  utils::mt_deque<int> queue;

  // push values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue]() {
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          int k = j;
          queue.push_tail(std::move(k));
        }
      });
    }

    JOIN_THREADS(threads);
  }

  // pop values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    std::vector<int> anss(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue, &anss, i]() {
        int ans = 0;
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          ans += *queue.try_pop_back();
        }
        anss[i] = ans;
      });
    }

    JOIN_THREADS(threads);

    int sum = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
      sum += anss[i];
    }

    EXPECT_EQ(sum, (ELEMENT_COUNT - 1) * ELEMENT_COUNT / 2 * THREAD_COUNT);
  }
}

TEST(mt_queue, push_front_pop_tail)
{
  std::vector<int> input = { 1, 7, 2, 3, 7, 5, 0, 4 };
  utils::mt_deque<int> queue;

  // push values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue]() {
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          int k = j;
          queue.push_front(std::move(k));
        }
      });
    }

    JOIN_THREADS(threads);
  }

  // pop values
  {
    std::vector<std::thread> threads(THREAD_COUNT);
    std::vector<int> anss(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++) {
      threads[i] = std::thread([&queue, &anss, i]() {
        int ans = 0;
        for (int j = 0; j < ELEMENT_COUNT; j++) {
          ans += *queue.try_pop_back();
        }
        anss[i] = ans;
      });
    }

    JOIN_THREADS(threads);

    int sum = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
      sum += anss[i];
    }

    EXPECT_EQ(sum, (ELEMENT_COUNT - 1) * ELEMENT_COUNT / 2 * THREAD_COUNT);
  }
}

} // namespace hnll