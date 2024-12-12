#pragma once

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "peer.h"

#define MAX_FILES 10
#define MAX_FILENAME_LEN 100

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

  ssize_t recv_buf(int s, int max) {
    buf.resize(max);
    ssize_t received = recv(s, buf.data(), buf.size(), 0);

    if (received < 0) {
      return received;
    }

    buf.resize(received);
    buf.shrink_to_fit();

    return received;
  }

  Peer handle_join(int peer_sfd) {
    uint32_t id;
    memcpy(&id, buf.data() + 1, sizeof(uint32_t));
    id = ntohl(id);

    return Peer(id, peer_sfd);
  }

  std::vector<std::string> handle_publish() {
    uint32_t count;
    memcpy(&count, buf.data() + 1, sizeof(uint32_t));
    count = ntohl(count);

    std::vector<std::string> files = {};
    size_t offset = sizeof(uint8_t) + sizeof(uint32_t);

    // Read each null-terminated string
    for (uint32_t i = 0; i < count; ++i) {
      size_t maxlen = std::min((size_t)MAX_FILENAME_LEN, buf.size() - offset);
      size_t len = strnlen((char*)buf.data() + offset, maxlen);

      std::string file((char*)buf.data() + offset, len);

      files.push_back(file);
      offset += len + 1;
    }


    return files;
  }

  std::string handle_search() {
    size_t maxlen = std::min((size_t)MAX_FILENAME_LEN, buf.size() - 1);
    size_t len = strnlen((char*)buf.data() + 1, maxlen);

    return std::string((char*)buf.data() + 1, len);
  }

  void search_response(Peer peer) {
    buf.resize(sizeof(uint32_t) * 2 + sizeof(uint16_t));
    uint32_t id = htonl(peer.id);
    uint32_t ip = peer.address.sin_addr.s_addr;
    uint16_t port = peer.address.sin_port;

    memcpy(buf.data(), &id, sizeof(uint32_t));
    memcpy(buf.data() + sizeof(uint32_t), &ip, sizeof(uint32_t));
    memcpy(buf.data() + sizeof(uint32_t) * 2, &port, sizeof(uint16_t));
  }
};