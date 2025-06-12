#define MAX_BUFFER_SIZE 2048

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/transport.h"

// Global flag to enable debug logging
static bool debug_enabled = false;

/**
 * enable_transport_debug - Enable or disable debug log printing
 *
 * @enable: true to enable debug logs, false to disable
 */
void enable_transport_debug(bool enable) {
	debug_enabled = enable;
}

/**
 * init_udp_socket - Create a UDP socket and optionally bind to a port
 *
 * @bind_ip: IP address to bind to (NULL for default)
 * @port: Port number to bind (0 for ephemeral)
 *
 * Return: UDP socket file descriptor, or -1 on failure
 */
int init_socket(const char* bind_ip, uint16_t port, bool is_server) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket() failed");
		return -1;
	}

	// bind if an IP or port is specified
	if (bind_ip != NULL || port != 0) {
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = (bind_ip != NULL) ? inet_addr(bind_ip) : INADDR_ANY;

		if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			perror("bind() failed");
			close(sockfd);
			return -1;
		}
	}
	return sockfd;
}

/**
 * send_bytes - Send a bytes buffer over UDP
 *
 * @sockfd: UDP socket file descriptor
 * @dest_addr: Pointer to destination sockaddr (IPV4)
 * @addrlen: Length of destination sockaddr
 * @buffer: Buffer to send
 * @len: size of buffer to send
 *
 * Return: Number of bytes sent, or -1 on error
 */
int send_bytes(int sockfd, const struct sockaddr* dest_addr, socklen_t addrlen, const uint8_t* buffer, size_t len) {
  if (debug_enabled) {
    printf("[SEND] %zu bytes -> %s:%d\n", len,
										inet_ntoa(((struct sockaddr_in*)dest_addr)->sin_addr),
										ntohs(((struct sockaddr_in*)dest_addr)->sin_port));
	}

  return sendto(sockfd, buffer, len, 0, dest_addr, addrlen);
}

/**
 * recv_bytes - Receive bytes into buffer from given sockfd over UDP
 *
 * @sockfd: UDP socket file descriptor
 * @buffer: Buffer to recieve
 * @max_len: Max length a buffer can hold
 * @from_addr: Output: sender's address
 * @from_len: Input/output: size of from_addr
 *
 * Return: 0 on success, -1 on error
 */
int recv_bytes(int sockfd, uint8_t* buffer, size_t max_len, struct sockaddr* from_addr, socklen_t* from_len) {
  ssize_t len = recvfrom(sockfd, buffer, max_len, 0, from_addr, from_len);
	if (len <= 0) return -1;

	if (debug_enabled) {
    char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((struct sockaddr_in*)from_addr)->sin_addr, ip, sizeof(ip));
		printf("[RECV] %ld bytes <- %s:%d\n", len, ip, ntohs(((struct sockaddr_in*)from_addr)->sin_port));
	}

	return (int)len;
}

