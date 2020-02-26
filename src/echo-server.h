#ifndef __ECHO_SERVER_H__
#define __ECHO_SERVER_H__

#include <poll.h>

#include <atomic>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "acceptor.h"
#include "socket.h"
#include "thread-pool.h"

namespace echo {
class EchoServer {
 public:
  EchoServer();
  EchoServer(const EchoServer&) = delete;
  EchoServer& operator=(const EchoServer&) = delete;
  virtual ~EchoServer();

  void Run(const PortType port);

 private:
  using FDArray = std::vector<pollfd>;
  using FDList = std::list<Descriptor>;
  using LockType = std::unique_lock<std::mutex>;

  static constexpr int kTimeout = 100;  // in milliseconds
  static constexpr int kMaxTaskQueueSize = 100;

  void Accept(const PortType port);
  void Echo(const Descriptor descriptor);
  void Dispatch();
  void Poll();

  std::atomic<bool> stop_requested_;
  std::condition_variable new_socket_;
  FDList pending_fds_;
  FDList ready_fds_;
  std::mutex mutex_;
  ThreadPool thread_pool_;
};
}  // namespace echo

#endif  // __ECHO_SERVER_H__
