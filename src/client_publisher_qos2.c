#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/slimmq_client.h"
#include "../include/slim_msg.h"

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
		fprintf(stderr, "[PUBLISHER QoS2] Failed to connect to broker.\n");
		return 1;
	}

	slimmq_set_qos(client, QOS_EXACTLY_ONCE);
	slimmq_set_retry_policy(client, 1000, 5);


	if (slimmq_publish(client, topic_str, payload, strlen(payload)) < 0) {
		fprintf(stderr, "[PUBLISHER QoS2] Failed to publish QoS2 message. \n");
				slimmq_close(client);
				return 1;
	}

	if (debug_mode) {
		printf("[PUBLISHER QoS2] Published to topic: %s with data: %s\n", topic_str, payload);
	}

	slimmq_close(client);
	return 0;
}
