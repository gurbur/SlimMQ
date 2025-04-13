#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "nanobus_client.h"
#include "transport.h"
#include "packet_handler.h"
#include "nano_msg.h"

#define MAX_PACKET_SIZE 2048

nanobus_client_t* nanobus_connect(const char* broker_ip, uint16_t port) {
    nanobus_client_t* client = calloc(1, sizeof(nanobus_client_t));
    if (!client) return NULL;

    client->sockfd = init_udp_socket(NULL, 0);  // ephemeral port
    if (client->sockfd < 0) {
        free(client);
        return NULL;
    }

    memset(&client->broker_addr, 0, sizeof(client->broker_addr));
    client->broker_addr.sin_family = AF_INET;
    client->broker_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, broker_ip, &client->broker_addr.sin_addr) <= 0) {
        close(client->sockfd);
        free(client);
        return NULL;
    }

    client->next_msg_id = 1;
    return client;
}

void nanobus_close(nanobus_client_t* client) {
    if (!client) return;
    close(client->sockfd);
    free(client);
}

int nanobus_subscribe(nanobus_client_t* client, const char* topic) {
    nano_msg_header_t header = {
        .version = 1,
        .msg_type = MSG_SUBSCRIBE,
        .qos_level = QOS_AT_MOST_ONCE,
        .msg_id = client->next_msg_id++,
        .payload_length = 1 + strlen(topic),
        .topic_id = 0,
        .frag_id = 0,
        .frag_total = 1,
        .batch_size = 1,
        .client_node_count = 1
    };

    uint8_t buffer[MAX_PACKET_SIZE];
    int len = serialize_message(&header, topic, NULL, 0, buffer, sizeof(buffer));
    if (len < 0) return -1;

    return send_bytes(client->sockfd, (struct sockaddr*)&client->broker_addr, sizeof(client->broker_addr), buffer, len);
}

int nanobus_publish(nanobus_client_t* client, const char* topic, const void* data, size_t data_len) {
    nano_msg_header_t header = {
        .version = 1,
        .msg_type = MSG_PUBLISH,
        .qos_level = QOS_AT_MOST_ONCE,
        .msg_id = client->next_msg_id++,
        .payload_length = 1 + strlen(topic) + data_len,
        .topic_id = 0,
        .frag_id = 0,
        .frag_total = 1,
        .batch_size = 1,
        .client_node_count = 1
    };

    uint8_t buffer[MAX_PACKET_SIZE];
    int len = serialize_message(&header, topic, data, data_len, buffer, sizeof(buffer));
    if (len < 0) return -1;

    return send_bytes(client->sockfd, (struct sockaddr*)&client->broker_addr, sizeof(client->broker_addr), buffer, len);
}

int nanobus_receive(nanobus_client_t* client, char* out_topic, size_t topic_buf_size, void* out_data, size_t data_buf_size) {
    uint8_t buffer[MAX_PACKET_SIZE];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    nano_msg_header_t header;

    int received = recv_bytes(client->sockfd, buffer, sizeof(buffer), (struct sockaddr*)&from, &fromlen);
    if (received < 0) return -1;

    return deserialize_message(buffer, received, &header, out_topic, topic_buf_size, out_data, data_buf_size);
}
