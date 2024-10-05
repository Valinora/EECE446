#include "utilities.h"

#include <stdint.h>

int recv_all(int socket, uint8_t* buff, ssize_t len) {
  int bytes_received;
  int total_received = 0;
  int leftover = len;

  while (leftover > 0) {
    bytes_received = recv(socket, buff + total_received, leftover, 0);
    if (bytes_received < 0) {
      // Return the error code
      return bytes_received;
    } else if (bytes_received == 0) {
      // Connection closed
      break;
    }
    total_received += bytes_received;
    leftover -= bytes_received;
  }

  return total_received;
}

int send_all(int s, char* buf, int len) {
  int total = 0;
  int bytesleft = len;
  int n;

  while (total < len) {
    n = send(s, buf + total, bytesleft, 0);
    if (n == -1) {
      // Return the error code
      return n;
    } else if (n == 0) {
      // Connection closed
      break;
    }
    total += n;
    bytesleft -= n;
  }

  return total;
}

int lookup_and_connect(const char* host, const char* service) {
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

string readline() {
  size_t len = 0;
  ssize_t read = 0;
  char* buf = NULL;
  read = getline(&buf, &len, stdin);

  if (read == -1) {
    perror("getline");
    free(buf);
    return (string){.buf = NULL, .len = 0};
  }

  // Replace the trailing newline from getline
  if (buf[read - 1] == '\n') {
    buf[read - 1] = '\0';
  }

  string out = (string){.buf = buf, .len = read};
  return out;
}

int32_t read_files(string** out) {
  DIR* dir = opendir("SharedFiles");

  if (dir == NULL) {
    fprintf(stderr, "Unable to open directory \"SharedFiles\". Exiting.\n");
    return -1;
  }

  struct dirent* entry;
  uint32_t count = 0;

  while ((entry = readdir(dir))) {
    // Ignore directories
    if (entry->d_type == DT_REG) {
      if (*out == NULL) {
        *out = malloc(sizeof(string));
      } else {
        *out = realloc(*out, (count + 1) * sizeof(string));
      }

      string filename = (string){.buf = strdup(entry->d_name), .len = strlen(entry->d_name) + 1};
      filename.buf[filename.len - 1] = '\0';
      (*out)[count] = filename;
      count++;
    }
  }

  closedir(dir);
  return count;
}