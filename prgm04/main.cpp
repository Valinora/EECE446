#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <optional>
#include <queue>

#define SERVER_PORT "5432"
#define MAX_LINE 256
#define MAX_PENDING 5

class PriorityQueueExt : public std::priority_queue<int> {
 public:
  /**
   * Removes a given value from the queue, regardless of position, and returns the value.
   * @param value The value to be removed.
   * @return A std::optional containing the value removed or nullopt if no match was found.
   */
  std::optional<int> remove(const int& value) {
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

  PriorityQueueExt conn_pq = {};

  // all_sockets stores all active sockets. Any socket connected to the server should
  // be included in the set. A socket that disconnects should be removed from the set.
  // The server's main socket should always remain in the set.
  fd_set all_sockets;
  FD_ZERO(&all_sockets);
  // call_set is a temporary used for each select call. Sockets will get removed from
  // the set by select to indicate each socket's availability.
  fd_set call_set;
  FD_ZERO(&call_set);

  // listen_socket is the fd on which the program can accept() new connections
  int listen_socket = bind_and_listen(port);
  conn_pq.push(listen_socket);
  FD_SET(listen_socket, &all_sockets);
  // max_socket should always contain the socket fd with the largest value, just one
  // for now.
  int max_socket = conn_pq.top();

  char buf[1024] = {0};

  while (1) {
    call_set = all_sockets;
    int num_s = select(max_socket + 1, &call_set, NULL, NULL, NULL);
    if (num_s < 0) {
      perror("ERROR in select() call");
      return -1;
    }
    // Check each potential socket.
    // Skip standard IN/OUT/ERROR -> start at 3.
    for (int s = 3; s <= max_socket; ++s) {
      // Skip sockets that aren't ready
      if (!FD_ISSET(s, &call_set)) continue;

      // A new connection is ready
      if (s == listen_socket) {
        int new_conn = accept(s, NULL, NULL);
        conn_pq.push(new_conn);
        FD_SET(new_conn, &all_sockets);
        printf("Socket %d connected\n", new_conn);

        max_socket = conn_pq.top();
      }

      // A connected socket is ready
      else {
        int received = recv(s, buf, 1024, 0);
        if (received < 0) {
          perror("received");
          return (-1);
        } else if (received == 0) {
          // Connection closed
          FD_CLR(s, &all_sockets);
          conn_pq.remove(s);
          max_socket = conn_pq.top();
          printf("Socket %d closed\n", s);
          continue;
        }

        printf("Socket %d sent: %s\n", s, buf);
      }
    }
  }
}

// Linear scan is bad!
int find_max_fd(const fd_set* fs) {
  int ret = 0;
  for (int i = FD_SETSIZE - 1; i >= 0 && ret == 0; --i) {
    if (FD_ISSET(i, fs)) {
      ret = i;
    }
  }
  return ret;
}

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
