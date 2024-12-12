#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <string>
#include <unordered_map>

#include "connpool.h"
#include "packet.h"
#include "peer.h"

#define MAX_LINE 256

// This is almost certainly too much memory
#define BUF_SIZE 1024 + MAX_FILES* MAX_FILENAME_LEN

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: <%s> [port]\n", argv[0]);
    return -1;
  }

  char* port = argv[1];

  ConnPool pool(port);
  std::unordered_map<int, Peer> peers = {};
  std::unordered_map<std::string, Peer> filemap = {};

  while (true) {
    for (const auto& ready_peer : pool.await()) {
      Packet packet;
      ssize_t received = packet.recv_buf(ready_peer, BUF_SIZE);

      if (received < 0) {
        fprintf(stderr, "Error receiving packet: %s\n", strerror(errno));
        continue;
      }

      // Conn closed, clean up.
      if (received == 0) {
        Peer& peer = peers[ready_peer];
        for (const auto& file : peer.files) {
          filemap.erase(file);
        }
        peers.erase(ready_peer);
        pool.releaseSocket(ready_peer);

        continue;
      }

      // If we made it to here, we have at least one byte. (received >= 1)
      switch (packet.buf[0]) {
        case JOIN: {
          Peer peer = packet.handle_join(ready_peer);
          peers[ready_peer] = peer;
          printf("TEST] JOIN %u\n", peer.id);
          break;
        }
        case SEARCH: {
          std::string search_term = packet.handle_search();

          // Returns default-constructed Peer if not found.
          Peer& peer = filemap[search_term];

          Packet response;
          response.search_response(peer);
          response.send_all(ready_peer);

          char ip[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &peer.address.sin_addr, ip, INET_ADDRSTRLEN);
          uint16_t port = ntohs(peer.address.sin_port);

          printf("TEST] SEARCH %s %u %s:%u\n", search_term.c_str(), peer.id, ip, port);
          break;
        }
        case PUBLISH: {
          auto files = packet.handle_publish();
          Peer& peer = peers.at(ready_peer);
          peer.add_files(files);

          printf("TEST] PUBLISH %zu ", files.size());
          for (const auto& file : files) {
            filemap[file] = peer;
            printf("%s ", file.c_str());
          }
          printf("\n");

          break;
        }
        default:
          printf("Unknown packet.\n");
          break;
      }
    }
  }
}
