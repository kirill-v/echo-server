#include <arpa/inet.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "socket.h"

void PrintHelp() {
  std::cout << "Usage: client [-h] -s SERVER_IP -p SERVER_PORT\n";
}

bool ParseArguments(int argc, char** argv, std::string& ip,
                    std::uint16_t& port) {
  int c;
  while ((c = getopt(argc, argv, "hp:s:")) != -1) {
    switch (c) {
      case 'h':
        PrintHelp();
        std::exit(0);
      case 'p':
        std::cout << "Port: " << optarg << std::endl;
        port = std::stoul(optarg);
        break;
      case 's':
        std::cout << "Server IP: " << optarg << std::endl;
        ip = optarg;
        break;
      case '?':
        std::cerr << "Unknown option: " << optopt << std::endl;
        return false;
      default:
        std::cerr << "Arguments parsing error\n";
        return false;
    }
  }

  if (port == 0) {
    std::cerr << "Error: Port is not set\n";
    return false;
  }
  if (ip.empty()) {
    std::cerr << "Error: Server IP address is not set\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  std::string ip;
  std::uint16_t port = 0;
  bool success = ParseArguments(argc, argv, ip, port);
  if (!success) {
    std::cerr << "Failed to parse arguments\n";
    PrintHelp();
    return 1;
  }

  try {
    echo::Socket socket(ip, port);
    std::string buffer;
    int message_size;
    while (std::cin.good()) {
      std::cout << "Enter test message: ";
      std::getline(std::cin, buffer);
      if (std::cin.eof()) {
        break;
      }
      if (!std::cin) {
        std::cerr << "Failed to read from stream\n";
        break;
      }
      message_size = buffer.size();
      socket.Write(buffer);
      buffer = socket.Read(message_size);
      std::cout << "Server response: " << buffer << std::endl;
    }
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}