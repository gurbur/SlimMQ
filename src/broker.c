#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "../include/transport.h"
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"
#include "../include/topic_table.h"

#define BROKER_PORT 9000

static bool debug_mode = false;

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
int receive_from_client(int sockfd, slim_msg_header_t* header, char* payload, struct sockaddr_in* client_addr, socklen_t* addrlen) {
	uint8_t buffer[2048];
	int received = recv_bytes(sockfd, buffer, sizeof(buffer), (struct sockaddr*)client_addr, addrlen);
	if (received < 0) return -1;

	return deserialize_message(buffer, received, header, payload, 256, NULL, 0);
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
int echo_to_client(int sockfd, const struct sockaddr_in* client_addr, socklen_t addrlen, const slim_msg_header_t* header, const void* payload) {
	uint8_t buffer[2048];

	int len = serialize_message(header, (const char*)payload, NULL, 0, buffer, sizeof(buffer));
	if (len < 0) return -1;

	int sent = send_bytes(sockfd, (const struct sockaddr*)client_addr, addrlen, buffer, len);

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
void handle_publish(int sockfd, const slim_msg_header_t* header, const char* topic_str) {
	SubscriberList* targets = get_matching_subscribers(topic_str);
	if (!targets) return;
	if (debug_mode) {
		printf("[BROKER] PUBLISH to %zu subscribers: %s\n", targets->count, topic_str);
	}

	for (Subscriber* s = targets->head; s != NULL; s = s->next) {
		uint8_t buffer[2048];

		int len = serialize_message(header, topic_str, NULL, 0, buffer, sizeof(buffer));
		if (len > 0){
			send_bytes(sockfd, (const struct sockaddr*)&s->addr, sizeof(s->addr), buffer, len);
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
		char payload[2048];

		int result = receive_from_client(sockfd, &header, payload, &client_addr, &addrlen);
		if (result != 0) {
			fprintf(stderr, "[BROKER] Failed to receive message.\n");
			continue;
		}

		payload[header.payload_length] = '\0';
		debug_dump_message(&header, payload);

		//echo_to_client(sockfd, &client_addr, addrlen, &header, payload);
		if (header.msg_type == MSG_SUBSCRIBE) {
			handle_subscribe(payload, &client_addr);
		} else if (header.msg_type == MSG_PUBLISH) {
			handle_publish(sockfd, &header, payload);
		}
		else {
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
