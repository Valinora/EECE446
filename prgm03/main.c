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

// Wish we had C23 on jaguar. Could specify enum size.
// `enum Action : uint8_t {` my beloved.
enum Action {
  JOIN = 0,
  PUBLISH,
  SEARCH,
  FETCH,  // New!
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

// New!
typedef struct {
  string filename;
} FetchBody;

// Thanks to padding, the bit layout here will not match our wire format.
// We'll still need to memcpy into a byte buffer.
// Though there's always __attribute__((packed))...
// Of course, that doesn't account for endianness.
// ¯\_(ツ)_/¯
typedef struct {
  enum Action tag;

  union {
    JoinBody join;
    PublishBody publish;
    SearchBody search;
    FetchBody fetch;  // New!
  } body;
} Packet;

/**
 * Represents a response to a search query.
 * If all fields are zero, the file was not found.
 * IP is stored in network byte order.
 */
typedef struct {
  uint32_t peer_id;
  uint32_t ip;
  uint16_t port;
} SearchResponse;

typedef struct {
  int8_t error;
  ptrdiff_t len;
  uint8_t* buf;
} FetchResponse;

/**
 * @brief Performs a peer-to-peer search based on the given search term.
 *
 * This function initiates a search to the registry for the given search term.
 *
 * @param search_term The term to search for in the peer-to-peer network.
 * @param s Network socket to send the search request on.
 * @return SearchResponse The result of the search, encapsulated in a SearchResponse structure.
 */
SearchResponse p2p_search(string search_term, int s);

FetchResponse p2p_fetch(string search_term, int s);

/**
 * Returns a pointer to an allocated buffer that contains
 * the network representation of a Packet.
 */
NetBuffer packet_to_netbuf(Packet packet);

void dump_packet(const NetBuffer* packet) {
  for (ssize_t i = 0; i < packet->len; i++) {
    printf("%02x ", packet->buf[i]);
  }
  printf("\n");
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <registry> <port_number> <peer_id>\n", argv[0]);
    return (EXIT_FAILURE);
  }

  int port = atoi(argv[2]);
  if (port < 2000 || port > 65535) {
    fprintf(stderr,
            "Invalid port number. Port must be between 2000 and 65535 "
            "inclusive.\n");
    return (EXIT_FAILURE);
  }

  // Signedness issue. TODO: Fix.
  uint32_t peer_id = atoi(argv[3]);

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

    if (strncmp(cmd_input.buf, "EXIT", 4) == 0) {
      exit = 1;
      break;
    }

    if (strncmp(cmd_input.buf, "JOIN", 4) == 0) {
      Packet packet = {.tag = JOIN, .body.join = {.peer_id = peer_id}};
      NetBuffer nb = packet_to_netbuf(packet);

      send_all(s, nb.buf, nb.len);
      free(nb.buf);
    }

    if (strncmp(cmd_input.buf, "SEARCH", 6) == 0) {
      printf("Please enter a filename to search for: ");
      string search_term = readline();

      SearchResponse response = p2p_search(search_term, s);

      if (response.peer_id == 0) {
        printf("File not indexed by registry.\n");
      } else {
        printf("File found at\n");
        char peer_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &response.ip, peer_ip, INET_ADDRSTRLEN);
        printf("\tPeer %u\n", response.peer_id);
        printf("\t%s:%d\n", peer_ip, response.port);
      }
    }

    if (strncmp(cmd_input.buf, "PUBLISH", 7) == 0) {
      string* files = NULL;
      int32_t count = read_files(&files);

      if (count < 0) {
        fprintf(stderr, "Failed to read files. Exiting.\n");
        return (EXIT_FAILURE);
      }

      Packet packet = {.tag = PUBLISH, .body.publish = {.count = count, .filenames = files}};
      NetBuffer nb = packet_to_netbuf(packet);

      send_all(s, nb.buf, nb.len);
    }

    // New!
    if (strncmp(cmd_input.buf, "FETCH", 5) == 0) {
      printf("Please enter a filename to fetch: ");
      string search_term = readline();

      FetchResponse response = p2p_fetch(search_term, s);

      if (response.error) {
        fprintf(stderr, "Failed to fetch file. Exiting.\n");
        return (EXIT_FAILURE);
      }

      FILE* f = fopen(search_term.buf, "wb");
      if (f == NULL) {
        fprintf(stderr, "Failed to open file for writing. Exiting.\n");
        return (EXIT_FAILURE);
      }

      fwrite(response.buf, 1, response.len, f);

      free(response.buf);
    }
    free(cmd_input.buf);
  }

  return 0;
}

SearchResponse p2p_search(string search_term, int s) {
  Packet packet = {.tag = SEARCH, .body.search = {.search_term = search_term}};
  NetBuffer nb = packet_to_netbuf(packet);

  ssize_t sent = send_all(s, nb.buf, nb.len);
  if (sent < 0) {
    fprintf(stderr, "Failed to send search term. Exiting.\n");
    return (SearchResponse){.peer_id = 0};
  }
  free(nb.buf);

  uint8_t response_buf[10];
  ssize_t received = recv_buffer(s, response_buf, 10);

  if (received < 0) {
    fprintf(stderr, "Failed to receive search response. Exiting.\n");
    return (SearchResponse){.peer_id = 0};
  }

  SearchResponse response;
  memcpy(&response.peer_id, response_buf, sizeof(uint32_t));
  memcpy(&response.ip, response_buf + sizeof(uint32_t), sizeof(uint32_t));
  memcpy(&response.port, response_buf + (2 * sizeof(uint32_t)), sizeof(uint16_t));
  response.peer_id = ntohl(response.peer_id);
  response.port = ntohs(response.port);

  return response;
}

int connect_to_peer(SearchResponse response) {
  char peer_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &response.ip, peer_ip, INET_ADDRSTRLEN);
  char port[6];
  snprintf(port, 6, "%d", response.port);
  return lookup_and_connect(peer_ip, port);
}

FetchResponse receive_file(int peer_s) {
  NetBuffer file = recv_all(peer_s);
  if (file.error) {
    fprintf(stderr, "Failed to receive file. Exiting.\n");
    return (FetchResponse){.error = 1};
  }

  FetchResponse fetch_response;
  memcpy(&fetch_response.error, file.buf, sizeof(int8_t));
  fetch_response.len = file.len - sizeof(int8_t);
  fetch_response.buf = malloc(fetch_response.len);
  if (fetch_response.buf == NULL) {
    fprintf(stderr, "Memory allocation failed. Exiting.\n");
    free(file.buf);
    return (FetchResponse){.error = 1};
  }
  memcpy(fetch_response.buf, file.buf + sizeof(int8_t), fetch_response.len);
  free(file.buf);

  return fetch_response;
}

FetchResponse p2p_fetch(string search_term, int s) {
  SearchResponse response = p2p_search(search_term, s);

  if (response.peer_id == 0) {
    return (FetchResponse){.error = 1};
  }

  int peer_s = connect_to_peer(response);

  if (peer_s < 0) {
    fprintf(stderr, "Failed to connect to peer. Exiting.\n");
    return (FetchResponse){.error = 1};
  }

  Packet packet = {.tag = FETCH, .body.fetch = {.filename = search_term}};
  NetBuffer nb = packet_to_netbuf(packet);
  send_all(peer_s, nb.buf, nb.len);
  free(nb.buf);

  NetBuffer file = recv_all(peer_s);
  if (file.error) {
    fprintf(stderr, "Failed to receive file. Exiting.\n");
    return (FetchResponse){.error = 1};
  }

  FetchResponse fetch_response = receive_file(peer_s);

  close(peer_s);
  return fetch_response;
}

// This would be a lot easier if we used a packed struct.
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
      break;
    case FETCH:  // New!
      size += packet.body.fetch.filename.len;
  }

  uint8_t* buffer = (uint8_t*)malloc(size);
  if (buffer == NULL) {
    return (NetBuffer){.buf = NULL, .len = 0};
  }

  uint8_t* ptr = buffer;
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
    case FETCH:  // New!
      memcpy(ptr, packet.body.fetch.filename.buf, packet.body.fetch.filename.len);
      break;
  }

  return (NetBuffer){.buf = buffer, .len = size};
}