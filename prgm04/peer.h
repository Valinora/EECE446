#pragma once

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class Peer {
 public:
  uint32_t id;
  int socket_fd;
  std::vector<std::string> files = {};
  struct sockaddr_in address;

  Peer() : id(0), socket_fd(0), address{} {}

  Peer(uint32_t id, int socket_fd) : id(id), socket_fd(socket_fd) {
    socklen_t len = sizeof(address);

    if (getpeername(socket_fd, (struct sockaddr*)&address, &len) != 0) {
      // Nothing good could have happened.
      // Unrecoverable.
      fprintf(stderr, "Error getting peer name: %s\n", strerror(errno));
      abort();
    }
  }

  void add_files(const std::vector<std::string>& files) {
    for (const auto& file : files) {
      add_file(file);
    }
  }

  void add_file(const std::string& file) { files.push_back(file); }

  bool operator==(const Peer& other) const { return socket_fd == other.socket_fd; }
};