#ifndef __ECHO_SERVER_H__
#define __ECHO_SERVER_H__

#include <errno.h>
#include <poll.h>

#include <atomic>
#include <chrono>
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
  EchoServer()
      : stop_requested_(false),
        thread_pool_(std::thread::hardware_concurrency(), kMaxTaskQueueSize) {}
  EchoServer(const EchoServer&) = delete;
  EchoServer& operator=(const EchoServer&) = delete;
  virtual ~EchoServer() {
    stop_requested_ = true;
    new_socket_.notify_all();  // to wake up dispatcher
    thread_pool_.Join();
    for (auto& fd : pending_fds_) {
      close(fd);
    }
    for (auto& fd : ready_fds_) {
      close(fd);
    }
  }

  void Run(const PortType port) {
    stop_requested_ = false;
    std::vector<std::thread> threads;
    threads.emplace_back([port, this]() { Accept(port); });
    threads.emplace_back([this]() { Dispatch(); });
    threads.emplace_back([this]() { Poll(); });
    for (auto& t : threads) {
      t.join();
    }
  }

 private:
  using FDArray = std::vector<pollfd>;
  using FDList = std::list<Descriptor>;
  using LockType = std::unique_lock<std::mutex>;

  static constexpr int kTimeout = 100;  // in milliseconds
  static constexpr int kMaxTaskQueueSize = 100;

  void Accept(const PortType port) {
    try {
      Acceptor acceptor(port);
      while (!stop_requested_) {
        auto fd = acceptor.Accept();
        LockType lock(mutex_);
        pending_fds_.emplace_back(fd);
        lock.unlock();
      }
    } catch (std::exception& e) {
      std::cerr << "Listening thread failed: " << e.what() << std::endl;
      stop_requested_ = true;
      new_socket_.notify_all();
    }
  }

  void Echo(const Descriptor descriptor) {
    // TODO: Use buffer pool to avoid repeated memory allocation
    Buffer temp(1024);
    Socket socket(descriptor, false);
    try {
      int bytes_number = socket.Read(temp, temp.size());
      if (bytes_number > 0) {
        std::cout << "Message from client: "
                  << std::string(reinterpret_cast<char*>(temp.data()),
                                 bytes_number)
                  << std::endl;
        socket.Write(temp, bytes_number);
        LockType lock(mutex_);
        pending_fds_.emplace_back(descriptor);
        lock.unlock();
      } else if (bytes_number == 0) {
        close(descriptor);
      }
    } catch (std::exception& e) {
      std::cerr << "Echo operation failed\n";
    }
  }

  void Dispatch() {
    try {
      FDList temp;
      while (!stop_requested_) {
        LockType lock(mutex_);
        if (ready_fds_.empty()) {
          new_socket_.wait(lock, [this]() {
            return !ready_fds_.empty() || stop_requested_;
          });
        }
        temp.splice(temp.end(), ready_fds_);
        lock.unlock();
        for (auto& fd : temp) {
          thread_pool_.RunTask([fd, this]() { Echo(fd); });
        }
        temp.clear();
      }
    } catch (std::exception& e) {
      std::cerr << "Dispatching thread failed: " << e.what() << std::endl;
      stop_requested_ = true;
    }
  }

  void Poll() {
    try {
      FDArray fds, temp_fds;
      while (!stop_requested_) {
        int ret = 0;
        if (fds.size() > 0) {
          ret = poll(&fds[0], fds.size(), kTimeout);
          if (ret == -1) {
            if (errno != EINTR) {
              perror("Poll");
              stop_requested_ = true;
              throw std::runtime_error("Polling thread failed");
            }
          }
        } else {
          auto t = std::chrono::milliseconds(kTimeout);
          std::this_thread::sleep_for(t);
        }

        LockType lock(mutex_);
        temp_fds.clear();
        int new_size_hint = fds.size() + pending_fds_.size() - ret;
        temp_fds.reserve(new_size_hint);
        for (auto& poll_fd : fds) {
          if (poll_fd.revents & POLLIN) {
            ready_fds_.push_back(poll_fd.fd);
          } else if ((poll_fd.revents & POLLERR) ||
                     (poll_fd.revents & POLLNVAL)) {
            std::cerr << "Some problems with socket " << poll_fd.fd
                      << std::endl;
            if (poll_fd.revents & POLLERR) {
              close(poll_fd.fd);
            }
          } else {
            temp_fds.push_back({poll_fd.fd, POLLIN, 0});
          }
        }
        for (auto& fd : pending_fds_) {
          temp_fds.push_back({fd, POLLIN, 0});
        }
        pending_fds_.clear();
        fds.swap(temp_fds);
        lock.unlock();
        new_socket_.notify_one();
      }
    } catch (std::exception& e) {
      std::cerr << "Polling thread failed: " << e.what() << std::endl;
      stop_requested_ = true;
      new_socket_.notify_all();
    }
  }

  std::atomic<bool> stop_requested_;
  std::condition_variable new_socket_;
  FDList pending_fds_;
  FDList ready_fds_;
  std::mutex mutex_;
  ThreadPool thread_pool_;
};
}  // namespace echo

#endif  // __ECHO_SERVER_H__
