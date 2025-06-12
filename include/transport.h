#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "slim_msg.h"

int init_socket(const char* bind_ip, uint16_t port);

int send_bytes(int sockfd, const struct sockaddr* dest_addr, socklen_t addrlen, const uint8_t* buffer, size_t len);

int recv_bytes(int sockfd, uint8_t* buffer, size_t max_len, struct sockaddr* from_addr, socklen_t* from_len);

void enable_transport_debug(bool enable);

