#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "../include/slimmq_client.h"
#include "../include/packet_handler.h"

#define DEFAULT_BROKER_PORT 9000;
#define DEFAULT_BROKER_IP "127.0.0.1"

static bool debug_mode = false;

void receive_message_loop(slimmq_client_t* client) {
	while(1) {
		char topic[128];
		void* data = NULL;
		size_t len = 0;

		if (slimmq_next_event(client, topic, sizeof(topic), &data, &len) == 0) {
			printf("[SUBSCRIBER QoS2] Topic: %s\n", topic);
			printf("[SUBSCRIBER QoS2] Data: %.*s\n", (int)len, (char*)data);
			free(data);
		} else {
			fprintf(stderr, "[SUBSCRIBE] Failed to receive event.\n");
		}
	}
}

int main(int argc, char* argv[]) {
	const char* topic_str = "sensor/+/temp";
	const char* broker_ip = DEFAULT_BROKER_IP;
	int port = DEFAULT_BROKER_PORT;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d")) {
			debug_mode = true;
			set_packet_debug(true);
		} else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
			topic_str = argv[++i];
		} else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
			broker_ip = argv[++i];
		} else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
			port = atoi(argv[++i]);
		}
	}

	slimmq_client_t* client = slimmq_connect(broker_ip, port);
	if (!client) {
		fprintf(stderr, "[SUBSCRIBER QoS2] Failed to connect to broker.\n");
		return 1;
	}

	slimmq_set_qos(client, QOS_EXACTLY_ONCE);

	if (slimmq_subscribe(client, topic_str) < 0) {
		fprintf(stderr, "[SUBSCRIBE QOS2] Failed to subscribe.\n");
		slimmq_close(client);
		return 1;
	}

	receive_message_loop(client);
	slimmq_close(client);
	return 0;
}
