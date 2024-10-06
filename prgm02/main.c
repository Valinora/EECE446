#include <arpa/inet.h>
#include <bits/types/struct_iovec.h>
#include <complex.h>
#include <dirent.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utilities.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

enum Action {
  JOIN = 0,
  PUBLISH,
  SEARCH,
};

typedef struct {
  uint32_t peer_id;
} JoinBody;

typedef struct {
  uint32_t count;
  string* filenames;
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

typedef struct {
  char* buf;
  size_t len;
} NetBuffer;

typedef struct {
  uint32_t peer_id;
  uint32_t ip;
  uint16_t port;
} SearchReponse;

/**
 * Returns a pointer to an allocated buffer that contains
 * the network representation of a Packet.
 */
NetBuffer packet_to_netbuf(Packet packet) {
#define qcopy(dst, x) memcpy(dst, &x, sizeof(x));
  size_t size = sizeof(uint8_t);
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
    return (NetBuffer){.buf = NULL, .len = 0};
  }

  char* ptr = buffer;
  memcpy(ptr, &packet.tag, sizeof(uint8_t));
  ptr += sizeof(uint8_t);

  switch (packet.tag) {
    case JOIN: {
      uint32_t peer_id = htonl(packet.body.join.peer_id);
      qcopy(ptr, peer_id);
      break;
    }
    case PUBLISH: {
      uint32_t count = htonl(packet.body.publish.count);
      qcopy(ptr, count);
      ptr += sizeof(count);
      for (uint32_t i = 0; i < packet.body.publish.count; i++) {
        ptrdiff_t len = packet.body.publish.filenames[i].len;
        memcpy(ptr, packet.body.publish.filenames[i].buf, len);
        ptr += len;
      }
      break;
    }
    case SEARCH:
      memcpy(ptr, packet.body.search.search_term.buf, packet.body.search.search_term.len);
      break;
  }

  return (NetBuffer){.buf = buffer, .len = size};
}

void dump_packet(const NetBuffer* packet) {
  for (size_t i = 0; i < packet->len; i++) {
    printf("%02x ", packet->buf[i]);
  }
  printf("\n");
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

  uint32_t peer_id = strtoul(argv[3], NULL, 10);

  if (peer_id <= 0) {
    fprintf(stderr, "Invalid peer id: \"%d\". Exiting.\n", peer_id);
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
      Packet packet = {.tag = JOIN, .body.join = {.peer_id = peer_id}};
      NetBuffer nb = packet_to_netbuf(packet);

      // dump_packet(&nb);
      send_all(s, nb.buf, nb.len);
      free(nb.buf);
    }

    if (strcmp(cmd_input.buf, "SEARCH") == 0) {
      printf("Please enter a filename to search for: ");
      // char* file_name = NULL;
      string search_term = readline();

      Packet packet = {.tag = SEARCH, .body.search = {.search_term = search_term}};
      NetBuffer nb = packet_to_netbuf(packet);

      // dump_packet(&nb);
      int sent = send_all(s, nb.buf, nb.len);
      if (sent < 0) {
        fprintf(stderr, "Failed to send search term. Exiting.\n");
        return (EXIT_FAILURE);
      }
      free(nb.buf);

      char* buf = malloc(10);
      if (buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for buffer. Exiting.\n");
        return (EXIT_FAILURE);
      }
      int received = recv_all(s, buf, 10);

      if (received < 0) {
        fprintf(stderr, "Failed to receive search response. Exiting.\n");
        return (EXIT_FAILURE);
      }

      SearchReponse response;
      memcpy(&response.peer_id, buf, sizeof(uint32_t));
      memcpy(&response.ip, buf + sizeof(uint32_t), sizeof(uint32_t));
      memcpy(&response.port, buf + (2 * sizeof(uint32_t)), sizeof(uint16_t));
      response.peer_id = ntohl(response.peer_id);
      response.port = ntohs(response.port);

      if (response.peer_id == 0) {
        printf("File not indexed by registry.\n");
      } else {
        printf("File found at\n");
        char* peer_ip = malloc(16);
        if (peer_ip == NULL) {
          fprintf(stderr, "Failed to allocate memory for peer ip. Exiting.\n");
          return (EXIT_FAILURE);
        }
        inet_ntop(AF_INET, &response.ip, peer_ip, 16);
        printf("\tPeer %u\n", response.peer_id);
        printf("\t%s:%d\n", peer_ip, response.port);
      }
    }

    if (strcmp(cmd_input.buf, "PUBLISH") == 0) {
      string* files = NULL;
      int32_t count = read_files(&files);

      if (count < 0) {
        fprintf(stderr, "Failed to read files. Exiting.\n");
        return (EXIT_FAILURE);
      }

      Packet packet = {.tag = PUBLISH, .body.publish = {.count = count, .filenames = files}};
      NetBuffer nb = packet_to_netbuf(packet);

      // dump_packet(&nb);
      send_all(s, nb.buf, nb.len);
    }

    free(cmd_input.buf);
  }

  return 0;
}
