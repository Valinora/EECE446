#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

#include "packet.h"
#include "connpool.h"

#define MAX_LINE 256
#define MAX_FILES 10
#define MAX_FILENAME_LEN 100
#define BUF_SIZE 1024 + MAX_FILES * MAX_FILENAME_LEN


/**
 * Create, bind and passive open a socket on a local interface for the provided service.
 * Argument matches the second argument to getaddrinfo(3).
 *
 * Returns a passively opened socket or -1 on error. Caller is responsible for calling
 * accept and closing the socket.
 */
int bind_and_listen(const char* service);

/**
 * Return the maximum socket descriptor set in the argument.
 * This is a helper function that might be useful to you.
 */
int find_max_fd(const fd_set* fs);

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: <" << argv[0] << "> [port]" << std::endl;
    return -1;
  }

  char* port = argv[1];

  ConnPool pool(port);
  PeerList peers;

  while(1) {
    for (const auto& ready_peer : pool.await()) {
      printf("Processing %d\n", ready_peer);
      std::vector<uint8_t> packet(BUF_SIZE);
      ssize_t received = recv(ready_peer, packet.data(), packet.size(), 0);
      packet.resize(received);

      if (received == 0) {
        printf("Socket %d closed\n", ready_peer);
        pool.releaseSocket(ready_peer);
        continue;
      }

      switch (packet[0]) {
        case JOIN:
          printf("JOIN\n");
          break;
        case SEARCH:
          printf("SEARCH\n");
          break;
        case PUBLISH:
          printf("PUBLISH\n");
          break;
        default:
          printf("Unknown packet.\n");
          break;
      }

    }
  }

}