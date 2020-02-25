#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  using TaskType = std::function<void()>;

  ThreadPool(const unsigned int threads, const unsigned int task_queue_size);
  virtual ~ThreadPool();

  // Waits for all the tasks to finish execution.
  void Join();
  // Runs task in separate thread. The method blocks if there is no free thread
  // to execute the task.
  void RunTask(TaskType&& task);

 private:
  using LockType = std::unique_lock<std::mutex>;

  void worker();

  // Maximum number of pending tasks
  const unsigned int kQueueMaxSize_;

  bool join_requested_;
  std::condition_variable free_worker_;
  std::condition_variable new_task_;
  std::mutex mutex_;
  std::queue<TaskType, std::list<TaskType>> tasks_;
  std::vector<std::thread> threads_;
};

#endif  // __THREAD_POOL_H__
