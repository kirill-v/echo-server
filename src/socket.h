#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <cstdint>
#include <string>
#include <vector>

namespace echo {
using Buffer = std::vector<std::uint8_t>;
using Descriptor = int;
using PortType = unsigned short;

static constexpr Descriptor kInvalidFD = -1;

class Socket {
 public:
  explicit Socket(const Descriptor descriptor, bool is_owner) noexcept;

  // Throws exception if
  // - server_ip string is ill formed
  // - fails to create socket
  // - fails to connect to the server
  Socket(const std::string &server_ip, const std::uint16_t port);
  Socket(Socket &&other) noexcept { *this = std::move(other); }
  Socket &operator=(Socket &&other) noexcept;
  virtual ~Socket() { Close(); }

  void Close();

  // Throws exception if
  // - socket descriptor is invalid
  // - reading operation fails with error code other than EINTR
  int Read(Buffer &buffer, const std::size_t bytes_to_read);
  // Use previous version internally
  std::string Read(std::size_t max_length);

  // Throws exception if
  // - socket descriptor is invalid
  // - writing operation fails with error code other than EINTR
  void Write(const Buffer &buffer, std::size_t bytes_to_write);
  // Use previous version internally
  void Write(const std::string &buffer);

 private:
  // Throws exception if socket descriptor is invalid.
  void Validate();

  Descriptor descriptor_;
  bool is_owner_;
};
}  // namespace echo

#endif  // __SOCKET_H__
