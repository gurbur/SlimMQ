#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdint.h>
#include "../include/transport.h"
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"
#include "../include/topic_table.h"
#include "../include/pending_table.h"

#define BROKER_PORT 9000
#define MAX_PACKET_SIZE 2048
#define FRAME_HEADER_SIZE 4

static bool debug_mode = false;

static int recv_framed_packet(int sockfd, uint8_t* buffer, size_t bufsize) {
    uint32_t packet_len = 0;
    int received = recv(sockfd, &packet_len, FRAME_HEADER_SIZE, MSG_WAITALL);
    if (received != FRAME_HEADER_SIZE) return -1;

    packet_len = ntohl(packet_len);
    if (packet_len > bufsize) return -1;

    received = recv(sockfd, buffer, packet_len, MSG_WAITALL);
    if (received != (int)packet_len) return -1;

    return received;
}

static int send_framed_packet(int sockfd, const uint8_t* data, size_t len) {
    uint32_t packet_len = htonl(len);
    if (send(sockfd, &packet_len, FRAME_HEADER_SIZE, 0) != FRAME_HEADER_SIZE) return -1;
    if (send(sockfd, data, len, 0) != (int)len) return -1;
    return 0;
}

void debug_dump_message(const slim_msg_header_t* header, const void* payload) {
    if (!debug_mode) return;

    printf("[BROKER] Received message:\n");
    dump_header(header);
    dump_payload(payload, header->payload_length);
}

void handle_subscribe(const char* topic_str, const struct sockaddr_in* client_addr) {
    subscribe_topic(topic_str, client_addr);
    if (debug_mode) {
        printf("[BROKER] Subscribed: %s\n", topic_str);
    }
}

void publish_to_subscribers(int sockfd, const slim_msg_header_t* header, const char* topic_str, const void* payload, size_t payload_length) {
    SubscriberList* targets = get_matching_subscribers(topic_str);
    if (!targets) return;

    if (debug_mode) {
        printf("[BROKER] PUBLISH to %zu subscribers: %s\n", targets->count, topic_str);
    }

    for (Subscriber* s = targets->head; s != NULL; s = s->next) {
        uint8_t buffer[MAX_PACKET_SIZE];
        int len = serialize_message(header, topic_str, payload, payload_length, buffer, sizeof(buffer));
        if (len > 0) {
            send_bytes(sockfd, (const struct sockaddr*)&s->addr, sizeof(s->addr), buffer, len);
        }
    }

    free_subscriber_list(targets);
}

void handle_publish(int sockfd, const slim_msg_header_t* header,
                    const char* topic_str, const void* payload, size_t payload_length,
                    const struct sockaddr_in* client_addr, socklen_t addrlen) {
    if (header->qos_level == QOS_EXACTLY_ONCE) {
        qos2_state_t state;
        if (pending_table_get(client_addr, header->msg_id, &state)) {
            if (state == QOS2_STATE_COMPLETED) {
                if (debug_mode) printf("[BROKER] QoS2 msg already completed, skipping\n");
                return;
            }
        } else {
            pending_table_update(client_addr, header->msg_id, QOS2_STATE_RECEIVED);
            publish_to_subscribers(sockfd, header, topic_str, payload, payload_length);
        }

        slim_msg_header_t ctrl_hdr = {
            .version = 1, .msg_type = MSG_CONTROL, .qos_level = QOS_EXACTLY_ONCE,
            .msg_id = header->msg_id, .payload_length = 1
        };

        uint8_t buffer[MAX_PACKET_SIZE];
        int len = serialize_control_message(&ctrl_hdr, CONTROL_RECEIVED, NULL, 0, buffer, sizeof(buffer));
        if (len > 0)
            send_framed_packet(sockfd, buffer, len);

        if (debug_mode) {
            printf("[BROKER] QoS2 -> Sent CONTROL_RECEIVED (msg_id=%u)\n", header->msg_id);
        }
        return;
    }

    if (header->qos_level == QOS_AT_LEAST_ONCE) {
        publish_to_subscribers(sockfd, header, topic_str, payload, payload_length);

        slim_msg_header_t ack_header = {
            .version = 1, .msg_type = MSG_ACK, .qos_level = QOS_AT_LEAST_ONCE,
            .msg_id = header->msg_id, .payload_length = 0
        };

        uint8_t ack_buf[MAX_PACKET_SIZE];
        int ack_len = serialize_message(&ack_header, NULL, NULL, 0, ack_buf, sizeof(ack_buf));
        if (ack_len > 0)
            send_framed_packet(sockfd, ack_buf, ack_len);

        if (debug_mode) {
            printf("[BROKER] Sent MSG_ACK for QoS1 msg_id=%u\n", header->msg_id);
        }
        return;
    }

    publish_to_subscribers(sockfd, header, topic_str, payload, payload_length);
}

void handle_control_release(int sockfd, const slim_msg_header_t* header, const struct sockaddr_in* client_addr, socklen_t addrlen) {
    pending_table_update(client_addr, header->msg_id, QOS2_STATE_RELEASED);

    slim_msg_header_t complete_hdr = {
        .version = 1, .msg_type = MSG_CONTROL, .qos_level = QOS_EXACTLY_ONCE,
        .msg_id = header->msg_id, .payload_length = 1
    };

    uint8_t buffer[MAX_PACKET_SIZE];
    int len = serialize_control_message(&complete_hdr, CONTROL_COMPLETE, NULL, 0, buffer, sizeof(buffer));
    if (len > 0)
        send_framed_packet(sockfd, buffer, len);

    pending_table_update(client_addr, header->msg_id, QOS2_STATE_COMPLETED);

    if (debug_mode) {
        printf("[BROKER] Received CONTROL_RELEASE -> Sent CONTROL_COMPLETE (msg_id=%u)\n", header->msg_id);
    }
}

void handle_control_received(int sockfd, const slim_msg_header_t* header, const uint8_t* payload, size_t payload_len, const struct sockaddr_in* client_addr, socklen_t addrlen) {
    qos2_state_t current;
    if (!pending_table_get(client_addr, header->msg_id, &current)) return;

    if (current != QOS2_STATE_RECEIVED) return;
    pending_table_update(client_addr, header->msg_id, QOS2_STATE_WAIT_RELEASE);

    if (debug_mode) {
        printf("[BROKER] CONTROL_RECEIVED acknowledged for msg_id=%u -> state=WAIT_RELEASE\n", header->msg_id);
    }
}

void handle_control(int sockfd, const slim_msg_header_t* header, const uint8_t* raw_buf, size_t buf_len, const struct sockaddr_in* client_addr, socklen_t addrlen) {
    control_type_t ctrl_type;
    char ctrl_data[2048];

    int ctrl_result = deserialize_control_message(raw_buf, buf_len, (slim_msg_header_t*)header, &ctrl_type, ctrl_data, sizeof(ctrl_data));
    if (ctrl_result != 0) {
        fprintf(stderr, "[BROKER] Failed to parse CONTROL message.\n");
        return;
    }

    switch (ctrl_type) {
        case CONTROL_RELEASE:
            handle_control_release(sockfd, header, client_addr, addrlen);
            break;
        case CONTROL_RECEIVED:
            handle_control_received(sockfd, header, (const uint8_t*)ctrl_data, sizeof(ctrl_data), client_addr, addrlen);
            break;
        default:
            if (debug_mode) {
                printf("[BROKER] Unhandled CONTROL type: %d\n", ctrl_type);
            }
            break;
    }
}

int init_broker_socket() {
    int sockfd = init_socket(NULL, BROKER_PORT, true);
    if (sockfd < 0) {
        fprintf(stderr, "[BROKER] Failed to create TCP socket.\n");
        return -1;
    }

    printf("[BROKER] Listening on port %d\n", BROKER_PORT);
    return sockfd;
}

void broker_main_loop(int listen_sockfd) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(listen_sockfd, (struct sockaddr*)&client_addr, &addrlen);
        if (connfd < 0) {
            perror("[BROKER] accept() failed");
            continue;
        }

        if (debug_mode) {
            printf("[BROKER] Accepted connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }

        while (1) {
            uint8_t buffer[MAX_PACKET_SIZE];
            int received = recv_framed_packet(connfd, buffer, sizeof(buffer));
            if (received <= 0) break;

            slim_msg_header_t header;
            char topic[256];
            char data[2048];

            int result = deserialize_message(buffer, received, &header, topic, sizeof(topic), data, sizeof(data));
            if (result != 0) continue;

            debug_dump_message(&header, data);

            if (header.msg_type == MSG_SUBSCRIBE) {
                handle_subscribe(topic, &client_addr);
            } else if (header.msg_type == MSG_PUBLISH) {
                handle_publish(connfd, &header, topic, data, header.payload_length - (1 + strlen(topic)), &client_addr, addrlen);
            } else if (header.msg_type == MSG_CONTROL) {
                handle_control(connfd, &header, buffer, received, &client_addr, addrlen);
            }
        }

        close(connfd);
        if (debug_mode) {
            printf("[BROKER] Closed client connection.\n");
        }
    }
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = true;
            enable_transport_debug(true);
            set_packet_debug(true);
        }
    }

    init_topic_table();
    int sockfd = init_broker_socket();
    if (sockfd < 0) return 1;
    pending_table_init();

    broker_main_loop(sockfd);

    free_topic_table();
    pending_table_destroy();
    close(sockfd);
    return 0;
}

