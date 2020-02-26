#include "socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace echo {
Socket::Socket(const Descriptor descriptor, bool is_owner) noexcept
    : descriptor_(descriptor), is_owner_(is_owner) {}  // namespace echo

Socket::Socket(const std::string &server_ip, const std::uint16_t port) {
  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  int ret = inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr);
  if (ret == -1) {
    throw std::invalid_argument("Failed to convert IP address to binary form");
  }
  descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
  if (descriptor_ == kInvalidFD) {
    throw std::runtime_error("Failed to create socket");
  }
  is_owner_ = true;
  ret = connect(descriptor_, reinterpret_cast<sockaddr *>(&server_address),
                sizeof(server_address));
  if (ret == -1) {
    throw std::runtime_error("Failed to connect to the server");
  }
}

Socket &Socket::operator=(Socket &&other) noexcept {
  descriptor_ = other.descriptor_;
  is_owner_ = other.is_owner_;
  other.descriptor_ = kInvalidFD;
  other.is_owner_ = false;
  return *this;
}

void Socket::Close() {
  if (descriptor_ != kInvalidFD) {
    if (is_owner_) {
      close(descriptor_);
    }
    descriptor_ = kInvalidFD;
    is_owner_ = false;
  }
}

int Socket::Read(Buffer &buffer, const std::size_t bytes_to_read) {
  Validate();

  if (buffer.size() < bytes_to_read) {
    buffer.resize(bytes_to_read);
  }

  int ret = -1;
  while (true) {
    ret = read(descriptor_, &buffer[0], bytes_to_read);
    if (ret == -1) {
      if (errno != EINTR) {
        throw std::runtime_error("Reading from socket failed");
      }
    } else {
      break;
    }
  }

  return ret;
}

std::string Socket::Read(std::size_t max_length) {
  Buffer temp(max_length);
  auto size = Read(temp, max_length);
  return std::move(std::string(reinterpret_cast<char *>(temp.data()), size));
}

void Socket::Write(const Buffer &buffer, std::size_t bytes_to_write) {
  Validate();

  int bytes_written = 0;
  if (bytes_to_write > buffer.size()) {
    bytes_to_write = buffer.size();
  }
  while (bytes_written < bytes_to_write) {
    int ret = write(descriptor_, &buffer[bytes_written],
                    bytes_to_write - bytes_written);
    if (ret == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("Writing to socket failed");
    }
    bytes_written += ret;
  }
}

void Socket::Write(const std::string &buffer) {
  Buffer temp(buffer.cbegin(), buffer.cend());
  Write(temp, temp.size());
}

void Socket::Validate() {
  if (descriptor_ == kInvalidFD) {
    throw std::invalid_argument("Invalid socket descriptor");
  }
}
}  // namespace echo