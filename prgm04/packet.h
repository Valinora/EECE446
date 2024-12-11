#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>
#include <vector>

enum Action {
  JOIN = 0,
  PUBLISH,
  SEARCH,
  FETCH,
};

class Packet {
 public:
  std::vector<std::uint8_t> buf;

  ssize_t send_all(int s) {
    size_t total = 0;
    ssize_t bytesleft = buf.size();
    ssize_t n;

    while (total < buf.size()) {
      n = send(s, buf.data() + total, bytesleft, 0);
      if (n == -1) {
        // Return the error code
        return n;
      } else if (n == 0) {
        // Connection closed
        break;
      }
      total += n;
      bytesleft -= n;
    }
    return total;
  }
  ssize_t recv_all(int s) {
    ssize_t total = 0;
    ssize_t bytesleft = 0;
    ssize_t n;

    while (true) {
      buf.resize(total + 1024);
      bytesleft = 1024;
      n = recv(s, buf.data() + total, bytesleft, 0);
      if (n < 0) {
        return n;
      } else if (n == 0) {
        // Peer has closed the connection
        break;
      }
      total += n;
    }

    buf.resize(total);
    return 0;
  }
};