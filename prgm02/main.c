#include "utilities.h"
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>

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
  char *filenames[];
} PublishBody;

typedef struct {
  char *search_term;
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

void packet_dump(const Packet *packet);
// size_t readline(char **buf);

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <registry> <port_number> <peer_id>\n", argv[0]);
    return (EXIT_FAILURE);
  }

  // Is there a way to check this without a useless conversion?
  int port = atoi(argv[2]);
  if (port < 2000 || port > 65535) {
    fprintf(stderr, "Invalid port number. Port must be between 2000 and 65535 "
                    "inclusive.\n");
    return (EXIT_FAILURE);
  }

  int peer_id = atoi(argv[3]);

  if (peer_id <= 0) {
    fprintf(stderr, "Invalid peer id: \"%d\". Exiting,\n", peer_id);
    return (EXIT_FAILURE);
  }

  int s;

  if ((s = lookup_and_connect(argv[1], argv[2])) < 0 ) {
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

void packet_dump(const Packet *packet) {
  const uint8_t *bytes = (const uint8_t *)packet;
  ssize_t size = sizeof(Packet);

  for (ssize_t i = 0; i < size; i++) {
    printf("%02x ", bytes[i]);
  }
  printf("\n");
}