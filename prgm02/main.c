#include "utilities.h"
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <registry> <port_number> <peer_id>\n", argv[0]);
    return (EXIT_FAILURE);
  }

  // Is there a way to check this without a useless conversion?
  int port = atoi(argv[2]);
  if (port < 2000 || port > 65535) {
    fprintf(stderr, "Invalid port number. Port must be between 2000 and 65535 inclusive.\n");
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