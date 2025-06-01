#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "../include/slimmq_client.h"
#include "../include/packet_handler.h"

#define DEFAULT_BROKER_PORT 9000
#define DEFAULT_BROKER_IP "127.0.0.1"

static bool debug_mode = false;

int main(int argc, char* argv[]) {
	const char* topic_str = "sensor/livingroom/temp";
	const char* payload = "23.5C";
	const char* broker_ip = DEFAULT_BROKER_IP;
	int port = DEFAULT_BROKER_PORT;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
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
		fprintf(stderr, "[PUBLISHER] Failed to connect to broker.\n");
		return 1;
	}

	if (slimmq_publish(client, topic_str, payload, strlen(payload)) < 0) {
		fprintf(stderr, "[PUBLISHER] Failed to publish.\n");
		slimmq_close(client);
		return 1;
	}

	if (debug_mode) {
		printf("[PUBLISHER] Published to topic: %s with data: %s\n", topic_str, payload);
	}

	slimmq_close(client);
	return 0;
}

