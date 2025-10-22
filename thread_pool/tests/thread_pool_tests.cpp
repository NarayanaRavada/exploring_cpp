/* Lambda-based TESTS rewritten by ChatGPT */

#include "../src/thread_pool.hpp"

#include <chrono>
#include <format>
#include <vector>
#include <future>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <cassert>
#include <iostream>
#include <functional>

int main() {
  using TestCase = std::pair<const char*, std::function<void()>>;

  std::vector<TestCase> tests = {
    {
      "test_post_after_shutdown", [] {
        std::future<void> fut;
        {
          ThreadPool pool(2);
          fut = post(pool, use_future([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
          }));
        }
        fut.get(); // will throw if it failed
      }
    },
    {
      "test_shutdown_with_long_task", [] {
        std::atomic<bool> task_done = false;
        {
          ThreadPool pool(2);
          post(pool, [&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            task_done = true;
          });
        }
        assert(task_done.load() && "Long task was not completed before shutdown");
      }
    },
    {
      "test_mixed_void_and_nonvoid", [] {
        ThreadPool pool(4);
        std::vector<std::future<int>> value_tasks;
        std::atomic<int> void_counter = 0;

        for (int i = 0; i < 10; ++i) {
          post(pool, [&void_counter] {
            void_counter.fetch_add(1, std::memory_order_relaxed);
          });

          value_tasks.push_back(post(pool, use_future([i] {
            return i * 10;
          })));
        }

        for (int i = 0; i < 10; ++i) {
          assert(value_tasks[i].get() == i * 10 && "Incorrect result from value task");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert(void_counter.load() == 10 && "Incorrect void task count");
      }
    },
    {
      "test_recursive_posting", [] {
        ThreadPool pool(2);
        std::atomic<int> counter = 0;

        post(pool, [&] {
          post(pool, [&] {
            counter.fetch_add(1);
          });
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert(counter.load() == 1 && "Recursive task posting failed");
      }
    },
    {
      "test_parallel_aggregation", [] {
        ThreadPool pool(4);
        std::vector<std::future<int>> futures;

        for (int i = 1; i <= 10; ++i) {
          futures.push_back(post(pool, use_future([i] {
            return i;
          })));
        }

        int sum = 0;
        for (auto& fut : futures) {
          sum += fut.get();
        }

        assert(sum == 55 && "Incorrect result from parallel aggregation");
      }
    },
    {
      "test_zero_threads", [] {
        bool threw = false;
        try {
          ThreadPool pool(0);
        } catch (...) {
          threw = true;
        }
        assert(threw && "ThreadPool should throw or handle zero thread case");
      }
    },
    {
      "test_move_only_task", [] {
        ThreadPool pool(2);
        auto fut = post(pool, use_future([]() -> std::unique_ptr<int> {
          return std::make_unique<int>(42);
        }));

        auto result = fut.get();
        assert(result && *result == 42 && "Move-only task failed");
      }
    },
    {
      "test_relative_ordering", [] {
        ThreadPool pool(1); // Single-threaded to ensure order
        std::vector<int> results;
        std::mutex lock;

        for (int i = 0; i < 5; ++i) {
          post(pool, [i, &results, &lock] {
            std::lock_guard<std::mutex> g(lock);
            results.push_back(i);
          });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (int i = 0; i < 5; ++i) {
          assert(results[i] == i && "Task order incorrect");
        }
      }
    },
  };

  // Run all tests
  for (size_t i = 0; i < tests.size(); ++i) {
    const auto& [name, test_fn] = tests[i];
    auto formattedName = std::format("{:<40}", name);
    try {
      test_fn();
      std::cout << formattedName << "PASS\n";
    } catch (const std::exception& e) {
      std::cerr << formattedName << "FAIL : "<< e.what() << '\n';
    } catch (...) {
      std::cerr << formattedName << "FAIL : Unknown exception\n";
    }
  }

  return 0;
}
