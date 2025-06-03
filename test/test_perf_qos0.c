#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/slimmq_client.h"
#include "../include/slim_msg.h"

#define COUNT 1000

int main(int argc, char* argv[]) {
	const char* ip = "127.0.0.1";
	int port = 9000;

	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "-ip") == 0) {
			ip = argv[i + 1];
		} else if (strcmp(argv[i], "-p") == 0) {
			port = atoi(argv[i + 1]);
		}
	}

	printf("[INFO] Connecting to %s:%d...\n", ip, port);

	slimmq_client_t* client = slimmq_connect(ip, (uint16_t)port);
	if (!client) {
		fprintf(stderr, "Failed to connect to broker\n");
		return 1;
	}

	slimmq_set_qos(client, QOS_AT_MOST_ONCE);

	for (int i = 0; i < COUNT; i++) {
		char msg[64];
		snprintf(msg, sizeof(msg), "qos0-message-%d", i);
		slimmq_publish(client, "test/perf", msg, strlen(msg));
	}

	printf("QoS 0: Sent %d messages (no confirmation)\n", COUNT);
	slimmq_close(client);
	return 0;
}

