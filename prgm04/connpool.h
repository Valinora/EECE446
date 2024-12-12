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

#include "netdb.h"
#include "peer.h"

#define MAX_PENDING 5


// Doing this is probably more work than just using a linear scan like I probably
// should have done, but what's the point of learning about data structures and algorithms
// if I don't use them?
class PriorityQueueExt : public std::priority_queue<int> {
 public:
  /**
   * Removes a given value from the queue, regardless of position, and returns the value.
   * @param value The value to be removed.
   * @return A std::optional containing the value removed or nullopt if no match was found.
   */
  inline std::optional<int> remove(const int& value) {
    auto it = std::find(this->c.begin(), this->c.end(), value);

    if (it == this->c.end()) {
      return std::nullopt;
    } else if (it == this->c.begin()) {
      int ret = this->top();
      this->pop();
      return ret;
    } else {
      int ret = *it;
      this->c.erase(it);
      std::make_heap(this->c.begin(), this->c.end(), this->comp);
      return ret;
    }
  }
};

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
  PriorityQueueExt conn_queue = {};
  fd_set all_sockets;
  int listen_socket;

 public:
  ConnPool(const char* service) {
    FD_ZERO(&all_sockets);
    listen_socket = bind_and_listen(service);
    FD_SET(listen_socket, &all_sockets);
    conn_queue.push(listen_socket);
  }

  ~ConnPool() {
    while(!conn_queue.empty()) {
      int s = conn_queue.top();
      conn_queue.pop();
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
    int num_s = select(conn_queue.top() + 1, &call_set, NULL, NULL, NULL);

    std::vector<int> active_sockets = {};

    if (num_s < 0) {
      std::cerr << "Error in select call" << std::endl;
      abort();
    }

    // Check each potential socket.
    // Skip standard IN/OUT/ERROR -> start at 3.
    for (int s = 3; s <= conn_queue.top(); ++s) {
      // Skip sockets that aren't ready
      if (!FD_ISSET(s, &call_set)) continue;

      // A new connection is ready
      if (s == listen_socket) {
        int new_conn = accept(s, NULL, NULL);
        conn_queue.push(new_conn);
        FD_SET(new_conn, &all_sockets);
      } else {
        active_sockets.push_back(s);
      }
    }
    return active_sockets;
  }

  void releaseSocket(int s) {
    FD_CLR(s, &all_sockets);
    conn_queue.remove(s);
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
