#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// INVARIANT: len will always include the null terminator.
typedef struct {
    char* buf;
    ptrdiff_t len;
} string;

/**
 * @brief Receives a specified number of bytes from a socket.
 *
 * This function attempts to read exactly `len` bytes from the given socket
 * and store them in the buffer `buff`. It continues to read from the socket
 * until the requested number of bytes have been received or an error occurs.
 *
 * @param s The file descriptor of the socket to read from.
 * @param buff A pointer to the buffer where the received data will be stored.
 * @param len The number of bytes to read from the socket.
 * @return The total number of bytes received on success, or -1 on error.
 */
int recv_all(int s, uint8_t* buff, ssize_t len);

/**
 * Sends all the data in the buffer through the specified socket.
 *
 * This function attempts to send the entire buffer by repeatedly calling
 * the send function until all bytes are sent or an error occurs.
 *
 * @param s The socket file descriptor.
 * @param buf The buffer containing the data to be sent.
 * @param len The length of the buffer.
 * @return The total number of bytes sent, or a negative value if an error occurs.
 */
int send_all(int s, char *buf, int len);

/**
 * Lookup a host IP address and connect to it using service. Arguments match the
 * first two arguments to getaddrinfo(3).
 *
 * @return a connected socket descriptor or -1 on error. Caller is responsible
 * for closing the returned socket.
 */
int lookup_and_connect(const char* host, const char* service);

/**
 * Wrapper around getline that replaces the trailing `\n` with a `\0`, shortening
 * input by 1.
 * @return String struct, with allocated memory, containing the user input.
 */
string readline();