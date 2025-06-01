#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>
#include "../include/transport.h"
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"
#include "../include/topic_table.h"

#define BROKER_PORT 9000
#define DEDUP_TABLE_SIZE 1024
#define DEDUP_EXPIRATION_SEC 10

static bool debug_mode = false;

typedef struct {
	struct sockaddr_in addr;
	uint32_t msg_id;
	time_t timestamp;
} dedup_entry_t;

static dedup_entry_t dedup_table[DEDUP_TABLE_SIZE];

/**
  * hash_dedup_key - simple hash function for deduplicating message
  */
static uint32_t hash_dedup_key(const struct sockaddr_in* addr, uint32_t msg_id) {
	return (addr->sin_addr.s_addr & addr->sin_port ^ msg_id) % DEDUP_TABLE_SIZE;
}

/**
 * is_duplicate_message - check if given message is duplicated or not
 */
bool is_duplicate_message(const struct sockaddr_in* addr, uint32_t msg_id) {
	uint32_t hash = hash_dedup_key(addr, msg_id);
	time_t now = time(NULL);

	for (int i = 0; i < DEDUP_TABLE_SIZE; ++i) {
		uint32_t idx = (hash + i) % DEDUP_TABLE_SIZE;
		dedup_entry_t* entry = &dedup_table[idx];

		if (entry->timestamp == 0) {
			entry->addr = *addr;
			entry->msg_id = msg_id;
			entry->timestamp = now;
			return false;
		}

		if (entry->msg_id == msg_id &&
				memcmp(&entry->addr, addr, sizeof(struct sockaddr_in)) == 0) {
			if (now - entry->timestamp < DEDUP_EXPIRATION_SEC) {
				return true;
			} else {
				entry->timestamp = now;
				return false;
			}
		}
	}

	dedup_table[hash].addr = *addr;
	dedup_table[hash].msg_id = msg_id;
	dedup_table[hash].timestamp = now;
	return false;
}

/**
 * init_broker_socket - create and bind a UDP socket for broker
 *
 * Return: the bound UDP socket file descriptor, or -1 on failure
 */
int init_broker_socket() {
	int sockfd = init_udp_socket(NULL, BROKER_PORT);
	if (sockfd < 0) {
		fprintf(stderr, "[BROKER] Failed to create UDP socket.\n");
		return -1;
	}

	printf("[BROKER] Listening on port %d\n", BROKER_PORT);
	return sockfd;
}

/**
 * receive_from_client - receive message from a client
 *
 * @sockfd: broker's UDP socket file descriptor
 * @header: output pointer to hold the received message header
 * @payload: buffer to hold the received message payload
 * @client_addr: output pointer to store client's address
 * @addrlen: pointer to length of address
 *
 * Return: 0 on success, -1 on failure
 */
int receive_from_client(int sockfd, slim_msg_header_t* header, char* payload,
												struct sockaddr_in* client_addr, socklen_t* addrlen) {
	uint8_t buffer[2048];
	int received = recv_bytes(sockfd, buffer, sizeof(buffer),
														(struct sockaddr*)client_addr,
														addrlen);
	if (received < 0) return -1;

	return deserialize_message(buffer, received,
															header, payload,
															256, NULL,
															0);
}

/**
 * debug_dump_message - print header and payload if debug mode is enabled
 *
 * @header: message header
 * @payload: raw payload buffer
 */
void debug_dump_message(const slim_msg_header_t* header, const void* payload) {
	if (!debug_mode) return;

	printf("[BROKER] Received message:\n");
	dump_header(header);
	dump_payload(payload, header->payload_length);
}

/**
 * echo_to_client - echo to client with given data
 *
 * @sockfd: broker's UDP socket file descriptor
 * @client_addr: address of client
 * @addrlen: length of address
 * @header: original message header
 * @payload: payload that goes up in message
 *
 * Return: 
 */
int echo_to_client(int sockfd, const struct sockaddr_in* client_addr,
										socklen_t addrlen, const slim_msg_header_t* header,
										const void* payload) {
	uint8_t buffer[2048];

	int len = serialize_message(header, (const char*)payload, NULL,
															0, buffer,
															sizeof(buffer));
	if (len < 0) return -1;

	int sent = send_bytes(sockfd, (const struct sockaddr*)client_addr,
												addrlen, buffer, len);

	if (debug_mode) {
		if (sent > 0) {
			printf("[BROKER] Echoed %d bytes back to client.\n", sent);
		} else {
			fprintf(stderr, "[BROKER] Failed to echo message.\n");
		}
	}

	return sent;
}

/**
 * handle_subscribe - handles a subscription request
 *
 * @topic_str: topic to subscribe to
 * @client_addr: address of subscribing client
 */
void handle_subscribe(const char* topic_str, const struct sockaddr_in* client_addr) {
	subscribe_topic(topic_str, client_addr);
	if (debug_mode) {
		printf("[BROKER] Subscribed: %s\n", topic_str);
	}
}

/**
 * handle_publish - handles a publish request and forwards it to matching subscribers
 *
 * @sockfd: UDP socket to send message
 * @header: original message header from the publisher
 * @topic_str: published topic string (used as payload here, for testing)
 */
void handle_publish(int sockfd, const slim_msg_header_t* header,
										const char* topic_str, const void* payload,
										size_t payload_length, const struct sockaddr_in* client_addr,
										socklen_t addrlen) {
	if (header->qos_level == QOS_AT_LEAST_ONCE) {
		if (is_duplicate_message(client_addr, header->msg_id)) {
			if (debug_mode) {
				printf("[BROKER] Duplicate QoS1 message detected. Skipping.\n");
			}
			slim_msg_header_t ack_header = {
				.version = 1,
				.msg_type = MSG_ACK,
				.qos_level = QOS_AT_LEAST_ONCE,
				.msg_id = header->msg_id,
				.topic_id = 0,
				.frag_id = 0,
				.frag_total = 1,
				.batch_size = 1,
				.payload_length = 0,
				.client_node_count = 1
			};
			uint8_t ack_buf[sizeof(slim_msg_header_t)];
			memcpy(ack_buf, &ack_header, sizeof(ack_header));
			send_bytes(sockfd, (const struct sockaddr*)client_addr,
									addrlen, ack_buf, sizeof(ack_buf));
			return;
		}
	}

	SubscriberList* targets = get_matching_subscribers(topic_str);
	if (!targets) return;
	if (debug_mode) {
		printf("[BROKER] PUBLISH to %zu subscribers: %s\n", targets->count,
						topic_str);
	}

	for (Subscriber* s = targets->head; s != NULL; s = s->next) {
		uint8_t buffer[2048];

		int len = serialize_message(header, topic_str, payload,
																payload_length, buffer,
																sizeof(buffer));
		if (len > 0){
			send_bytes(sockfd, (const struct sockaddr*)&s->addr,
									sizeof(s->addr), buffer, len);
		}
	}

	free_subscriber_list(targets);
}

/**
 * broker_main_loop - main loop of the broker
 *
 * @sockfd: udp socket of broker
 */
void broker_main_loop(int sockfd) {
	while(1) {
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		slim_msg_header_t header;
		char topic[256];
		char data[2048];

		uint8_t buffer[2048];
		int received = recv_bytes(sockfd, buffer, sizeof(buffer),
															(struct sockaddr*)&client_addr,
															&addrlen);

		if (received < 0) {
			fprintf(stderr, "[BROKER] Failed to receive bytes\n");
			continue;
		}

		int result = deserialize_message(buffer, received,
																			&header, topic,
																			sizeof(topic),
																			data, sizeof(data));

		if (result != 0) {
			fprintf(stderr, "[BROKER] Failed to deserialize message.\n");
			continue;
		}

		debug_dump_message(&header, data);

		if (header.msg_type == MSG_SUBSCRIBE) {
			handle_subscribe(topic, &client_addr);
		} else if (header.msg_type == MSG_PUBLISH) {
			handle_publish(sockfd, &header, topic, data,
											header.payload_length - (1 + strlen(topic)),
											&client_addr, addrlen);

			if (header.qos_level == QOS_AT_LEAST_ONCE) {
				slim_msg_header_t ack_header = {
					.version = 1,
					.msg_type = MSG_ACK,
					.qos_level = QOS_AT_LEAST_ONCE,
					.msg_id = header.msg_id,
					.topic_id = 0,
					.frag_id = 0,
					.frag_total = 1,
					.batch_size = 1,
					.payload_length = 0,
					.client_node_count = 1
				};

				uint8_t ack_buf[sizeof(slim_msg_header_t)];
				memcpy(ack_buf, &ack_header, sizeof(ack_header));

				send_bytes(sockfd, (struct sockaddr*)&client_addr, addrlen,
										ack_buf, sizeof(ack_buf));

				if (debug_mode) {
					printf("[BROKER] Sent MSG_ACK for QoS1 msg_id=%u\n",
									header.msg_id);
				}
			}
		}	else {
			if (debug_mode) {
				printf("[BROKER] Unknown message type: %d\n", header.msg_type);
			}
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

	broker_main_loop(sockfd);

	free_topic_table();
	close(sockfd);
	return 0;
}
