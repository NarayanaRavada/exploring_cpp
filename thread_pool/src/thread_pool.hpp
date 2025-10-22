#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool {
public:
  explicit ThreadPool(int threadCount = std::thread::hardware_concurrency())
  : isActive(true)
  {
    if (threadCount == 0) {
      throw std::runtime_error("Thread count must be greater than \"0\"");
    }
    for (int i = 0; i < threadCount; i++) {
      pool.emplace_back(&ThreadPool::run, this);
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(gaurd);
      isActive = false;
    }

    cv.notify_all();
    for (auto &th: pool) {
      if (th.joinable()) th.join();
    }

    // execute pendingJobs
    while (!pendingJobs.empty()) {
      auto job = std::move(pendingJobs.front());
      pendingJobs.pop_front();
      job();
    }
  }

  void post(std::packaged_task<void()> job) {
    std::unique_lock<std::mutex> lock(gaurd);
    if (!isActive) {
      throw std::runtime_error("Thread pool has been shutdown, new jobs cannot be posted");
    }
    pendingJobs.emplace_back(std::move(job));
    lock.unlock();
    cv.notify_one();
  }

  void run() noexcept {
    while (isActive) {
      std::packaged_task<void()> job;
      {
        std::unique_lock lock(gaurd);
        cv.wait(lock, [&] { return !pendingJobs.empty() || !isActive; });
        if(!isActive && pendingJobs.empty()) break;
        job = std::move(pendingJobs.front());
        pendingJobs.pop_front();
      }
      job();
    }
  }


private:
  std::atomic_bool isActive;
  std::vector<std::thread> pool;
  std::deque<std::packaged_task<void()>> pendingJobs;

  std::condition_variable cv;
  std::mutex gaurd;
};


// a warapper for both fn and std::use_future(fn)

template <class Executor, class Fn>
void post(Executor &&exec, Fn &&fn) {
  using return_type = std::invoke_result_t<Fn>;
  static_assert(std::is_void_v<return_type>, "function with non_void return types must be used with \"use future\" tag");
  std::packaged_task<void()> job(std::forward<Fn>(fn));
  exec.post(std::move(job));
}

struct use_future_tag {};
template <class Fn>
auto use_future(Fn &&fn) {
  return std::make_tuple(use_future_tag{}, std::forward<Fn>(fn));
}

template <class Executor, class Fn>
decltype(auto) post(Executor &&exec, std::tuple<use_future_tag, Fn> &&tup) {
  using return_type = std::invoke_result_t<Fn>;
  auto [_, fn] = tup;
  if constexpr (std::is_void_v<return_type>) {
    std::packaged_task<void()> job(std::move(fn));
    auto ret_future = job.get_future();
    exec.post(std::move(job));
    return ret_future;
  } else {
    auto promise = std::make_shared<std::promise<return_type>>();
    auto ret_future = promise->get_future();
    std::packaged_task<void()> job(
      [promise, fn = std::forward<Fn>(fn)] () mutable {
        try {
          promise->set_value(fn());
        } catch (...) {
          promise->set_exception(std::current_exception());
        }
      }
    );

    exec.post(std::move(job));
    return ret_future;
  }
}
