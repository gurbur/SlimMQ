#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "../include/transport.h"
#include "../include/nano_msg.h"
#include "../include/packet_handler.h"
#include "../include/topic_table.h"

#define BROKER_PORT 9000

static bool debug_mode = false;

int init_broker_socket() {
	int sockfd = init_udp_socket(NULL, BROKER_PORT);
	if (sockfd < 0) {
		fprintf(stderr, "[BROKER] Failed to create UDP socket.\n");
		return -1;
	}

	printf("[BROKER] Listening on port %d\n", BROKER_PORT);
	return sockfd;
}

int receive_from_client(int sockfd, nano_msg_header_t* header, char* payload, struct sockaddr_in* client_addr, socklen_t* addrlen) {
	return recv_message(sockfd, header, payload, 2048,
			(struct sockaddr*)client_addr, addrlen);
}

void debug_dump_message(const nano_msg_header_t* header, const void* payload) {
	if (!debug_mode) return;

	printf("[BROKER] Received message:\n");
	dump_header(header);
	dump_payload(payload, header->payload_length);
}

int echo_to_client(int sockfd, const struct sockaddr_in* client_addr, socklen_t addrlen, const nano_msg_header_t* header, const void* payload) {
	int sent = send_message(sockfd, (const struct sockaddr*)client_addr, addrlen, header, payload);

	if (debug_mode) {
		if (sent > 0) {
			printf("[BROKER] Echoed %d bytes back to client.\n", sent);
		} else {
			fprintf(stderr, "[BROKER] Failed to echo message.\n");
		}
	}

	return sent;
}

void handle_subscribe(const char* topic_str, const struct sockaddr_in* client_addr) {
	subscribe_topic(topic_str, client_addr);
	if (debug_mode) {
		printf("[BROKER] Subscribed: %s\n", topic_str);
	}
}

void handle_publish(int sockfd, const nano_msg_header_t* header, const char* topic_str) {
	SubscriberList* targets = get_matching_subscribers(topic_str);

	if (debug_mode) {
		printf("[BROKER] PUBLISH to %zu subscribers: %s\n", targets->count, topic_str);
	}

	for (Subscriber* s = targets->head; s != NULL; s = s->next) {
		send_message(sockfd, (struct sockaddr*)&s->addr, sizeof(s->addr), header, topic_str);
	}

	free_subscriber_list(targets);
}

void broker_main_loop(int sockfd) {
	while(1) {
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		nano_msg_header_t header;
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
