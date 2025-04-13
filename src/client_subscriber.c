#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/transport.h"
#include "../include/nano_msg.h"
#include "../include/packet_handler.h"

#define DEFAULT_BROKER_PORT 9000
#define DEFAULT_BROKER_IP "127.0.0.1"
#define MAX_PAYLOAD_LEN 256

static bool debug_mode = false;

void send_subscribe_request(int sockfd, const struct sockaddr_in* broker_addr, const char* topic_str) {
    nano_msg_header_t header = {
        .version = 1,
        .msg_type = MSG_SUBSCRIBE,
        .qos_level = QOS_AT_MOST_ONCE,
        .topic_id = 0,  // Not used for now
        .msg_id = 100,
        .frag_id = 0,
        .frag_total = 1,
        .batch_size = 1,
        .payload_length = strlen(topic_str),
        .client_node_count = 1
    };

    int sent = send_message(sockfd, (const struct sockaddr*)broker_addr, sizeof(*broker_addr), &header, topic_str);
    if (sent < 0) {
        fprintf(stderr, "[CLIENT] Failed to send SUBSCRIBE.\n");
        return;
    }

    if (debug_mode) {
        printf("[CLIENT] SUBSCRIBE sent for topic: %s\n", topic_str);
    }
}

void receive_messages_loop(int sockfd) {
    while (1) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        nano_msg_header_t header;
        char payload[MAX_PAYLOAD_LEN];

        int res = recv_message(sockfd, &header, payload, sizeof(payload), (struct sockaddr*)&from, &fromlen);
        if (res == 0) {
            payload[header.payload_length] = '\0';
            printf("[RECV] Message on topic: %s\n", payload);
            dump_header(&header);
            dump_payload(payload, header.payload_length);
        } else {
            fprintf(stderr, "[CLIENT] Failed to receive message.\n");
        }
    }
}

int main(int argc, char* argv[]) {
    const char* topic_str = "sensor/+/temp";
    const char* broker_ip = DEFAULT_BROKER_IP;
    int port = DEFAULT_BROKER_PORT;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = true;
            enable_transport_debug(true);
            set_packet_debug(true);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            topic_str = argv[++i];
        } else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            broker_ip = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

    int sockfd = init_udp_socket(NULL, 0);
    if (sockfd < 0) {
        fprintf(stderr, "[CLIENT] Failed to create socket.\n");
        return 1;
    }

    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    send_subscribe_request(sockfd, &broker_addr, topic_str);
    receive_messages_loop(sockfd);

    close(sockfd);
    return 0;
}

