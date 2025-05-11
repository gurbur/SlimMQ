#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "../include/transport.h"
#include "../include/packet_handler.h"
#include "../include/slim_msg.h"

#define DEFAULT_BROKER_PORT 9000
#define DEFAULT_BROKER_IP "127.0.0.1"
#define MAX_PAYLOAD_LEN 256

static bool debug_mode = false;

void send_echo_message(int sockfd, const struct sockaddr_in* broker_addr, const char* msg) {
    slim_msg_header_t header = {
        .version = 1,
        .msg_type = MSG_DATA,
        .qos_level = QOS_AT_MOST_ONCE,
        .topic_id = 1001,
        .msg_id = 1000,
        .frag_id = 0,
        .frag_total = 1,
        .batch_size = 1,
        .payload_length = strlen(msg),
        .client_node_count = 1
    };

    int sent = send_message(sockfd, (const struct sockaddr*)broker_addr, sizeof(*broker_addr), &header, msg);
    if (sent < 0) {
        fprintf(stderr, "[CLIENT] Failed to send message.\n");
        return;
    }

    if (debug_mode) {
        printf("[CLIENT] Sent %d bytes to broker.\n", sent);
    }

    // receive
    struct sockaddr_in recv_from;
    socklen_t from_len = sizeof(recv_from);
    slim_msg_header_t recv_header;
    char recv_payload[MAX_PAYLOAD_LEN];

    int status = recv_message(sockfd, &recv_header, recv_payload, sizeof(recv_payload),
                              (struct sockaddr*)&recv_from, &from_len);
    if (status != 0) {
        fprintf(stderr, "[CLIENT] Failed to receive echo.\n");
        return;
    }

    dump_header(&recv_header);
    dump_payload(recv_payload, recv_header.payload_length);

    printf("[CLIENT] Echo message: '%.*s'\n", recv_header.payload_length, recv_payload);
}

int main(int argc, char* argv[]) {
    const char* message = "Hello, SlimMQ!";
    const char* broker_ip = DEFAULT_BROKER_IP;
    int port = DEFAULT_BROKER_PORT;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = true;
            enable_transport_debug(true);
            set_packet_debug(true);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[++i];
        } else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            broker_ip = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

    // UDP socket generation
    int sockfd = init_udp_socket(NULL, 0);
    if (sockfd < 0) {
        fprintf(stderr, "[CLIENT] Failed to create UDP socket.\n");
        return 1;
    }

    // broker address setting
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    send_echo_message(sockfd, &broker_addr, message);

    close(sockfd);
    return 0;
}

