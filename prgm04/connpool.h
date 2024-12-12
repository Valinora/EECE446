#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <queue>
#include <optional>
#include <algorithm>
#include <set>

#include "netdb.h"
#include "peer.h"

#define MAX_PENDING 5

/**
 * Create, bind and passive open a socket on a local interface for the provided service.
 * Argument matches the second argument to getaddrinfo(3).
 *
 * Returns a passively opened socket or -1 on error. Caller is responsible for calling
 * accept and closing the socket.
 */
int bind_and_listen(const char* service);

class ConnPool {
 protected:
  std::set<int> conn_set = {};
  fd_set all_sockets;
  int listen_socket;

 public:
  ConnPool(const char* service) {
    FD_ZERO(&all_sockets);
    listen_socket = bind_and_listen(service);
    FD_SET(listen_socket, &all_sockets);
    conn_set.insert(listen_socket);
  }

  ~ConnPool() {
    for (int s : conn_set) {
      close(s);
    }
  }

  /**
   * @brief Waits for activity on any of the sockets and returns a list of active sockets.
   *
   * @return std::vector<int> A vector containing the file descriptors that are ready for I/O.
   *
   * @note If an error occurs during the `select` call, the function will print an error message
   *       and abort the program.
   */
  std::vector<int> await() {
    fd_set call_set = all_sockets;
    int max_fd = *conn_set.rbegin();
    int num_s = select(max_fd + 1, &call_set, NULL, NULL, NULL);

    std::vector<int> active_sockets = {};

    if (num_s < 0) {
      std::cerr << "Error in select call: " << strerror(errno) << std::endl;
      std::cerr << "select(" << max_fd + 1 << ", &call_set, NULL, NULL, NULL) -> " << errno << std::endl;
      abort();
    }

    for (int s : conn_set) {
      if (!FD_ISSET(s, &call_set)) {
        continue;
      }
      if (s == listen_socket) {
        int new_conn = accept(s, NULL, NULL);
        conn_set.insert(new_conn);
        FD_SET(new_conn, &all_sockets);
      } else {
        active_sockets.push_back(s);
      }
    }
    return active_sockets;
  }

  void releaseSocket(int s) {
    FD_CLR(s, &all_sockets);
    conn_set.erase(s);
    close(s);
  }
};

int bind_and_listen(const char* service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Build address data structure */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;

  /* Get local address info */
  if ((s = getaddrinfo(NULL, service, &hints, &result)) != 0) {
    fprintf(stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }

  /* Iterate through the address list and try to perform passive open */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
      continue;
    }

    if (!bind(s, rp->ai_addr, rp->ai_addrlen)) {
      break;
    }

    close(s);
  }
  if (rp == NULL) {
    perror("stream-talk-server: bind");
    return -1;
  }
  if (listen(s, MAX_PENDING) == -1) {
    perror("stream-talk-server: listen");
    close(s);
    return -1;
  }
  freeaddrinfo(result);

  return s;
}
