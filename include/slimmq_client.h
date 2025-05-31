#pragma once

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>
#include <pthread.h>
#include "event_queue.h"

/**
 * slimMQ client context structure
 */
typedef struct {
	int sockfd;												// internal UDP socket
	struct sockaddr_in broker_addr;		// destination broker address
	uint32_t next_msg_id;							// incremental message ID generator
	slimmq_event_queue_t event_queue;	// event queue
	pthread_t listener_thread;				// thread for incomming messages
	int running;											// flag for thread loop control
	int qos_level;										// QoS level for publish
	int retry_timeout_ms;							// time out millisecond for qos 1/2
	int max_retries;									// max retry num for qos 1/2
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

/**
 * slimmq_start_listener - start event queue's listener thread
 *
 * @client: slimMQ client
 *
 * Return: 
 */
int slimmq_start_listener(slimmq_client_t* client);

/**
 * slimmq_next_event - pop a message from event queue
 *
 * @client: slimMQ client
 * @out_topic: topic buffer
 * @topic_buf_size: size of topic buffer
 * @out_data: payload buffer
 * @out_data_len: size of payload buffer
 *
 * Return: 
 */
int slimmq_next_event(slimmq_client_t* client, char* out_topic, size_t topic_buf_size, void** out_data, size_t* out_data_len);


/**
 * slimmq_set_qos - set QoS level in given client
 *
 * @client: client pointer to set qos level
 * @qos_level: enum for wanted qos level
 */
void slimmq_set_qos(slimmq_client_t* client, uint8_t qos_level);

