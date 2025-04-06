#define MAX_BUFFER_SIZE 2048

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/transport.h"
#include "../include/nano_msg.h"
#include "../include/packet_handler.h"

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
int init_udp_socket(const char* bind_ip, uint1_t port) {
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
 * send_message - Send a message over UDP (header + payload)
 *
 * @sockfd: UDP socket file descriptor
 * @dest_addr: Pointer to destination sockaddr (IPV4)
 * @addrlen: Length of destination sockaddr
 * @header: Pointer to nanoBUS message header
 * @payload: Pointer to payload data (can be NULL if empty)
 *
 * Return: Number of bytes sent, or -1 on error
 */
int send_message(int sockfd, const struct sockaddr* dest_addr, socklen_t addrlen, const nano_msg_header_t* header, const void* payload) {
	uint8_t buffer[MAX_BUFFER_SIZE];
	int len = serialize_message(header, payload, buffer, sizeof(buffer));
	if (len < 0) {
		fprintf(stderr, "Failed to serialize message.\n");
		return -1;
	}

	if (debug_enabled) {
		printf("[SEND] %d bytes -> %s:%d\n", len,
			inet_ntoa(((struct sockaddr_in*)dest_addr)->sin_addr),
			ntohs(((struct sockaddr_if*)dest_addr)->sin_port));
	}

	return sendto(sockfd, buffer, len, 0, dest_addr, addrlen);
}

/**
 * recv_message - Receive and parse a UDP message into header + payload
 *
 * @sockfd: UDP socket file descriptor
 * @out_header: Output pointer to parsed header structure
 * @out_payload: Output buffer to hold received payload
 * @max_payload_len: Maximum allowed size for payload
 * @from_addr: Output: sender's address
 * @from_len: Input/output: size of from_addr
 *
 * Return: 0 on success, -1 on error
 */
int recv_message(int sockfd, nano_msg_header_t* out_header, void* out_payload, size_t struct sockaddr* from_addr, socklen_t* from_len) {
	uint8_t buffer[MAX_BUFFER_SIZE];
	ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, from_addr, from_len);

	if (len <= 0) {
		return -1;
	}

	if (debug_enabled) {
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((struct sockaddr_in*)from_addr)->sin_addr, ip, sizeof(ip));
		printf("[RECV] %ld bytes <- %s:%d\n", len, ip, ntohs(((struct sockaddr_in*)from_addr)->sin_port));
	}

	return deserialize_message(buffer, len, out_header, out_payload, max_payload_len);
}

