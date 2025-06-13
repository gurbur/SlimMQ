#define MAX_BUFFER_SIZE 2048

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/transport.h"

static bool debug_enabled = false;

void enable_transport_debug(bool enable) {
    debug_enabled = enable;
}

int init_socket(const char* bind_ip, uint16_t port, bool is_server) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (bind_ip != NULL) ? inet_addr(bind_ip) : INADDR_ANY;

    if (is_server) {
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind() failed");
            close(sockfd);
            return -1;
        }

        if (listen(sockfd, 10) < 0) {
            perror("listen() failed");
            close(sockfd);
            return -1;
        }

        if (debug_enabled)
            printf("[SERVER] Listening on port %d...\n", port);

        return sockfd;

    } else {
        if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("connect() failed");
            close(sockfd);
            return -1;
        }

        if (debug_enabled)
            printf("[CLIENT] Connected to %s:%d\n",
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        return sockfd;
    }
}

int send_bytes(int sockfd, const struct sockaddr* dest_addr, socklen_t addrlen,
               const uint8_t* buffer, size_t len) {
    (void)dest_addr;
    (void)addrlen;

    if (debug_enabled)
        printf("[SEND] %zu bytes via TCP\n", len);

    return send(sockfd, buffer, len, 0);
}

int recv_bytes(int sockfd, uint8_t* buffer, size_t max_len,
               struct sockaddr* from_addr, socklen_t* from_len) {
    (void)from_addr;
    (void)from_len;

    ssize_t len = recv(sockfd, buffer, max_len, 0);
    if (len < 0) {
        perror("[RECV ERROR]");
        return -1;
    } else if (len == 0) {
        if (debug_enabled)
            printf("[RECV] Connection closed by peer\n");
        return -1;
    }

    if (debug_enabled)
        printf("[RECV] %ld bytes via TCP\n", len);

    return (int)len;
}

