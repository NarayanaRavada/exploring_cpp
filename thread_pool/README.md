# ThreadPool

A minimal, lightweight C++17 thread pool that supports asynchronous task execution, with optional `std::future` support for tasks that return values.

- Supports both:
  - Fire-and-forget tasks (no return value).
  - Future-based tasks with return values.

## Usage

### Creating a Thread Pool

```cpp
ThreadPool pool(4); \\ creates pool with 4 threads
```

### Posting a Fire-and-Forget task

```cpp
post(pool, [] {
    std::cout << "Running a background task\n";
});
```

### Posting a Task with a return value

If you need to get the result of a task (i.e., it returns a value), use the `use_feature` wrapper to post the task and get a future.

```cpp
auto future = post(pool, use_feature([] {
    return 42;
}));

int result = future.get(); // result == 42
```

Posting functions with non-void return types without `use_feature` will trigger a static assertion.

## Design Notes

- All tasks are wrapped in `std::packaged_task<void()>` for uniform handling, even for return-value tasks

- Thread Safety: Uses a `std::mutex` and `std::condition_variable` to guard the job queue.

- Worker Lifecycle:
	- Threads wait for work via a condition variable.
	- Cleanly shut down on destruction by flipping an std::atomic_bool flag and notifying all threads.

- Cleanly shut down on destruction by flipping an std::atomic_bool flag and notifying all threads. current implementation runs the remaining tasks in destructor thread.
