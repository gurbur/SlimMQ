#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "slim_msg.h"

int init_udp_socket(const char* bind_ip, uint16_t port);

int send_message(int sockfd, const struct sockaddr* dest_addr, socklen_t addrlen, const slim_msg_header_t* header, const void* payload);

int recv_message(int sockfd, slim_msg_header_t* out_header, void* out_payload, size_t max_payload_len, struct sockaddr* from_addr, socklen_t* from_len);

void enable_transport_debug(bool enable);

