#pragma once

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

/**
 * slimMQ client context structure
 */
typedef struct {
    int sockfd;                         // internal UDP socket
    struct sockaddr_in broker_addr;    // destination broker address
    uint32_t next_msg_id;              // incremental message ID generator
} slimmq_client_t;

/**
 * slimmq_connect - Create and initialize a UDP client connection to broker
 *
 * @broker_ip: IPv4 string (e.g. "127.0.0.1")
 * @port: UDP port number of the broker
 *
 * Return: pointer to allocated slimmq_client_t, or NULL on failure
 */
slimmq_client_t* slimmq_connect(const char* broker_ip, uint16_t port);

/**
 * slimmq_close - Close the UDP socket and free the client context
 */
void slimmq_close(slimmq_client_t* client);

/**
 * slimmq_subscribe - Send a SUBSCRIBE request to broker for a topic
 */
int slimmq_subscribe(slimmq_client_t* client, const char* topic);

/**
 * slimmq_publish - Publish a message to a given topic
 */
int slimmq_publish(slimmq_client_t* client,
                    const char* topic,
                    const void* data,
                    size_t data_len);

/**
 * slimmq_receive - Receive a message (blocking)
 *
 * Extracts both topic string and data from the received message
 */
int slimmq_receive(slimmq_client_t* client,
                    char* out_topic, size_t topic_buf_size,
                    void* out_data, size_t data_buf_size);

