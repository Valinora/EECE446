#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>

#include "packet.h"
#include "peerpool.h"

#define SERVER_PORT "5432"
#define MAX_LINE 256
#define MAX_PENDING 5


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

  PeerPool pool(port);

  while(1) {
    for (const auto& ready_peer : pool.await()) {
      printf("Processing %d\n", ready_peer);
      Packet packet;
      ssize_t received = packet.recv_all(ready_peer, 1024);

      if (received == 0) {
        printf("Socket %d closed\n", ready_peer);
        pool.releaseSocket(ready_peer);
      }

      for (const auto& byte : packet.buf) {
        printf("%02x ", byte);
      }
      printf("\n");
    }
  }

}