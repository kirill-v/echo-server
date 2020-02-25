#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include "socket.h"

namespace echo {

class Acceptor {
 public:
  Acceptor(const PortType port) {
    descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor_ == -1) {
      throw std::runtime_error("Failed to open socket");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    auto ret = bind(descriptor_, reinterpret_cast<sockaddr *>(&address),
                    sizeof(address));
    if (ret == -1) {
      Close();
      throw std::runtime_error("Failed to bind socket to port");
    }

    ret = listen(descriptor_, kQueueSize);
    if (ret == -1) {
      Close();
      throw std::runtime_error("Failed to start listen for socket connections");
    }
  }
  virtual ~Acceptor() { Close(); }

  // Throws exception if
  // - socket descriptor is invalid
  // - internal call to accept fails
  // - conversion of IP from binary format to text fails
  Descriptor Accept() {
    Validate();

    sockaddr_in client_address;
    socklen_t address_length = sizeof(client_address);
    int descriptor =
        accept(descriptor_, reinterpret_cast<sockaddr *>(&client_address),
               &address_length);
    if (descriptor == -1) {
      throw std::runtime_error("Failed to accept connection on a socket");
    }
    constexpr auto kAddressLength = 16;
    Buffer client_ip(kAddressLength + 1);
    auto ret =
        inet_ntop(AF_INET, &client_address.sin_addr,
                  reinterpret_cast<char *>(&client_ip[0]), kAddressLength);
    if (ret == nullptr) {
      throw std::runtime_error("Failed to convert ip from binary to text form");
    }
    std::cout << "Accepted connection from " << client_ip.data() << ":"
              << ntohs(client_address.sin_port) << std::endl;

    return descriptor;
  }

  void Close() {
    if (descriptor_ != kInvalidFD) {
      close(descriptor_);
      descriptor_ = kInvalidFD;
    }
  }

 private:
  static constexpr auto kQueueSize = 10;
  // Throws exception if socket descriptor is invalid.
  void Validate() {
    if (descriptor_ == kInvalidFD) {
      throw std::invalid_argument("Invalid socket descriptor");
    }
  }
  Descriptor descriptor_;
};
}  // namespace echo

#endif  // __ACCEPTOR_H__