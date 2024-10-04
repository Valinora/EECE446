#include <complex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utilities.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

enum Action : uint8_t {
  JOIN = 0,
  PUBLISH,
  SEARCH,
};

typedef struct {
  uint32_t peer_id;
} JoinBody;

typedef struct {
  uint32_t count;
  string filenames[];
} PublishBody;

typedef struct {
  string search_term;
} SearchBody;

// Thanks to padding, the bit layout here will not match our wire format.
// We'll still need to memcpy into a byte buffer.
typedef struct {
  enum Action tag;

  union {
    JoinBody join;
    PublishBody publish;
    SearchBody search;
  } body;
} Packet;

/**
 * Returns a pointer to an allocated buffer that contains
 * the network representation of a Packet.
 */
char* packettons(Packet packet) {
#define qcopy(dst, x) memcpy(dst, &x, sizeof(x));
  size_t size = sizeof(packet.tag);
  switch (packet.tag) {
    case JOIN:
      size += sizeof(packet.body.join.peer_id);
      break;
    case PUBLISH:
      size += sizeof(packet.body.publish.count);
      for (uint32_t i = 0; i < packet.body.publish.count; i++) {
        size += packet.body.publish.filenames[i].len;
      }
      break;
    case SEARCH:
      size += packet.body.search.search_term.len;
  }

  char* buffer = (char*)malloc(size);
  if (buffer == NULL) {
    return NULL;
  }

  char* ptr = buffer;
  qcopy(ptr, packet.tag);
  ptr += sizeof(packet.tag);

  switch (packet.tag) {
    case JOIN:
      qcopy(ptr, packet.body.join.peer_id);
      break;
    case PUBLISH:
      qcopy(ptr, packet.body.publish.count);
      for (uint32_t i = 0; i < packet.body.publish.count; i++) {
        ptrdiff_t len = packet.body.publish.filenames[i].len;
        memcpy(ptr, packet.body.publish.filenames[i].buf, len);
        ptr += len;
      }
      break;
    case SEARCH:
      memcpy(ptr, packet.body.search.search_term.buf, packet.body.search.search_term.len);
      break;
  }

  return buffer;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <registry> <port_number> <peer_id>\n", argv[0]);
    return (EXIT_FAILURE);
  }

  // Is there a way to check this without a useless conversion?
  int port = atoi(argv[2]);
  if (port < 2000 || port > 65535) {
    fprintf(stderr,
            "Invalid port number. Port must be between 2000 and 65535 "
            "inclusive.\n");
    return (EXIT_FAILURE);
  }

  int peer_id = atoi(argv[3]);

  if (peer_id <= 0) {
    fprintf(stderr, "Invalid peer id: \"%d\". Exiting,\n", peer_id);
    return (EXIT_FAILURE);
  }

  int s;

  if ((s = lookup_and_connect(argv[1], argv[2])) < 0) {
    fprintf(stderr, "Unable to connect to host \"%s\". Exiting.\n", argv[1]);
    return (EXIT_FAILURE);
  }

  int exit = 0;

  while (!exit) {
    printf("Please enter a command, one of JOIN, SEARCH, PUBLISH, or EXIT: ");
    string cmd_input = readline();

    // printf("%s\n", cmd_input);

    if (strcmp(cmd_input.buf, "EXIT") == 0) {
      exit = 1;
      break;
    }

    if (strcmp(cmd_input.buf, "JOIN") == 0) {
      // do join
    }

    if (strcmp(cmd_input.buf, "SEARCH") == 0) {
      printf("Please enter a filename to search for: ");
      // char* file_name = NULL;
      string file_name = readline();

      printf("%s\n", file_name.buf);
    }

    if (strcmp(cmd_input.buf, "PUBLISH") == 0) {
      // do publish
    }

    free(cmd_input.buf);
  }

  return 0;
}