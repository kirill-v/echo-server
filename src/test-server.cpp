#include <string>

#include "echo-server.h"

void PrintHelp() { std::cout << "Usage: server SERVER_PORT\n"; }

int main(int argc, char** argv) {
  if (argc != 2) {
    PrintHelp();
    return 1;
  }
  try {
    auto port = std::stoul(argv[1]);
    echo::EchoServer server;
    std::cout << "Starting server on port " << port << std::endl;
    server.Run(9999);
  } catch (std::exception& e) {
    std::cerr << "Echo server failed: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}