#pragma once

#include <cstdint>
#include <vector>
#include <sys/types.h>

enum Action {
  JOIN = 0,
  PUBLISH,
  SEARCH,
  FETCH,
};

class Packet {
 public:
  std::vector<std::uint8_t> buf;

  ssize_t send_all(int s);
  int recv_all(int s);
};