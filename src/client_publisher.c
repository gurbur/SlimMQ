#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/transport.h"
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"

#define DEFAULT_BROKER_PORT 9000
#define DEFAULT_BROKER_IP "127.0.0.1"
#define MAX_PAYLOAD_LEN 256

static bool debug_mode = false;

void send_publish(int sockfd, const struct sockaddr_in* broker_addr,
                  const char* topic_str) {
    slim_msg_header_t header = {
        .version = 1,
        .msg_type = MSG_PUBLISH,
        .qos_level = QOS_AT_MOST_ONCE,
        .topic_id = 0,      // 미사용
        .msg_id = 101,      // 고정값 or 추후 증가 방식
        .frag_id = 0,
        .frag_total = 1,
        .batch_size = 1,
        .payload_length = strlen(topic_str),
        .client_node_count = 1
    };

    int sent = send_message(sockfd, (const struct sockaddr*)broker_addr,
                            sizeof(*broker_addr), &header, topic_str);
    if (sent < 0) {
        fprintf(stderr, "[PUBLISHER] Failed to send message.\n");
        return;
    }

    if (debug_mode) {
        printf("[PUBLISHER] Sent topic: %s\n", topic_str);
    }
}

int main(int argc, char* argv[]) {
    const char* topic_str = "sensor/livingroom/temp";
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
        fprintf(stderr, "[PUBLISHER] Failed to create socket.\n");
        return 1;
    }

    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    send_publish(sockfd, &broker_addr, topic_str);

    close(sockfd);
    return 0;
}

