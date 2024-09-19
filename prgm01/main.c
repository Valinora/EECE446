/* This code is an updated version of the sample code from "Computer Networks: A
 * Systems Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some
 * code comes from man pages, mostly getaddrinfo(3). */
#include <assert.h>
#include <limits.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Lookup a host IP address and connect to it using service. Arguments match the
 * first two arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible
 * for closing the returned socket.
 */
int lookup_and_connect(const char *host, const char *service);
int pattern_count(const char *buffer, const char *pattern, int length);

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <window size>\n", argv[0]);
    return -1;
  }

  int window_size = atoi(argv[1]);

  if (window_size <= 3) {
    printf("Window size must be an integer >= 3.\n");
    return -1;
  }

  int s;
  const char *host = "www.ecst.csuchico.edu";
  const char *port = "80";
  char HTTP_REQ[] = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";

  char window[window_size + 1];
  char read_buf[window_size];

  /* Lookup IP and connect to server */
  if ((s = lookup_and_connect(host, port)) < 0) {
    exit(1);
  }

  ssize_t bytes_sent = 0;
  ssize_t total_bytes_sent = 0;
  ssize_t request_len = sizeof(HTTP_REQ);

  while (total_bytes_sent < request_len) {
    bytes_sent =
        send(s, HTTP_REQ + total_bytes_sent, request_len - total_bytes_sent, 0);
    if (bytes_sent <= 0) {
      perror("send");
      close(s);
      return -1;
    }
    total_bytes_sent += bytes_sent;
  }

  ssize_t bytes_received = INT_MAX;
  int total_bytes = 0;
  int tag_count = 0;
  int leftover = window_size;


  while ( bytes_received > 0) {
    bytes_received = recv(s, read_buf, leftover, 0);
    total_bytes += bytes_received;
    memcpy(window + (window_size - leftover), read_buf, bytes_received);
    leftover -= bytes_received;

    if(leftover == 0 || bytes_received == 0) {
      tag_count += pattern_count(window, "<h1>", window_size);
      leftover = window_size;
    }


  }


  printf("Number of <h1> tags: %d\n", tag_count);
  printf("Number of bytes: %d\n", total_bytes);
  close(s);

  return 0;
}


/**
 * Counts the occurrences of `pattern` in `buffer`.
 */
int pattern_count(const char *buffer, const char *pattern, int length) {
  int count = 0;
  const char *start = buffer;
  const char *end = buffer + length;
  while ((start = strstr(start, pattern)) != NULL && start < end) {
    count++;
    // Move past the current match
    start += strlen(pattern);
  }

  return count;
}

int lookup_and_connect(const char *host, const char *service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Translate host name into peer's IP address */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ((s = getaddrinfo(host, service, &hints, &result)) != 0) {
    fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }

  /* Iterate through the address list and try to connect */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
      continue;
    }

    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }

    close(s);
  }
  if (rp == NULL) {
    perror("stream-talk-client: connect");
    return -1;
  }
  freeaddrinfo(result);

  return s;
}
