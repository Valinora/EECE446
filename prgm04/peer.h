#pragma once

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

#include <cstdint>
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
      abort();
    }
  }

  void add_files(const std::vector<std::string>& files) {
    for (const auto& file : files) {
      add_file(file);
    }
  }

  void add_file(const std::string file) { files.push_back(file); }

  bool operator==(const Peer& other) const { return socket_fd == other.socket_fd; }
};

class PeerList {
  std::unordered_map<std::string, Peer> filemap = {};
  // I'd love to use peer->id as the index into this, but std::vector isn't sparse,
  // and peer->id can be arbitrarily large.
  // Could use another hash map, but ¯\_(ツ)_/¯
  std::vector<Peer> peers = {};

 public:
  std::optional<Peer> find(std::string filename) {
    if (filemap.count(filename) < 1) {
      return {};
    }

    return filemap.at(filename);
  }

  void add(Peer peer) {
    for (const auto& file : peer.files) {
      filemap.at(file) = peer;
    }

    peers.push_back(peer);
  }

  // I generally dislike the std's API design of not returning elements that it removes.
  // I may never use this return value, but I'll be glad to have it if I *do* end up needing it.
  std::optional<Peer> remove(Peer peer) {
    for (const auto& file : peer.files) {
      filemap.erase(file);
    }

    std::optional<Peer> out = {};
    for (auto it = peers.begin(); it != peers.end();) {
      if (*it == peer) {
        out = *it;
        peers.erase(it);
      } else {
        ++it;
      }
    }

    return out;
  }

  /**
   * Removes a peer from the list, given a socket file descriptor.
   * Performs a linear scan to find the matching peer, and calls
   * `Peer::remove(Peer)` to do the actual removal.
   */
  std::optional<Peer> remove(int sfd) {
    Peer needle(0, 0);
    // All that talk about hash maps and O(1) lookups,
    // and yet I still end up doing linear scans...........
    for (const auto& peer : peers) {
      if (peer.socket_fd == sfd) {
        needle = peer;
      }
    }

    return remove(needle);
  }
};