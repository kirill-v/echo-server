#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include "socket.h"

namespace echo {

class Acceptor {
 public:
  Acceptor(const PortType port);
  virtual ~Acceptor() { Close(); }

  // Throws exception if
  // - socket descriptor is invalid
  // - internal call to accept fails
  // - conversion of IP from binary format to text fails
  Descriptor Accept();
  void Close();

 private:
  static constexpr int kQueueSize = 10;
  // Throws exception if socket descriptor is invalid.
  void Validate();
  Descriptor descriptor_;
};
}  // namespace echo

#endif  // __ACCEPTOR_H__