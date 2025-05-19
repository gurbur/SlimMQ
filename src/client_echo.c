#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "../include/slimmq_client.h"
#include "../include/packet_handler.h"

#define DEFAULT_BROKER_PORT 9000
#define DEFAULT_BROKER_IP "127.0.0.1"

static bool debug_mode = false;

void send_echo_message(slimmq_client_t* client, const char* msg) {
	slimmq_subscribe(client, "echo");

	usleep(100 * 1000);

	slimmq_publish(client, "echo", msg, strlen(msg));

	if (debug_mode) {
		printf("[CLIENT] Sent echo: '%s'\n", msg);
	}

	char topic[128];
	void* data = NULL;
	size_t len = 0;

	while (1) {
		if (slimmq_next_event(client, topic, sizeof(topic), &data, &len) == 0) {
			if (strcmp(topic, "echo") == 0) {
				printf("[CLIENT] Echo received on [%s]: %.*s\n", topic, (int)len, (char*)data);
				free(data);
				break;
			} else {
				if(debug_mode) {
					printf("[CLIENT] Ignoring non-echo message: [%s]\n", topic);
				}
				free(data);
			}
		}
	}
}

int main(int argc, char* argv[]) {
    const char* message = "Hello, SlimMQ!";
    const char* broker_ip = DEFAULT_BROKER_IP;
    int port = DEFAULT_BROKER_PORT;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_mode = true;
            set_packet_debug(true);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[++i];
        } else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            broker_ip = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

		slimmq_client_t* client = slimmq_connect(broker_ip, port);
		if (!client) {
			fprintf(stderr, "[CLIENT] Failed to connect to broker.\n");
			return 1;
		}

		send_echo_message(client, message);

		slimmq_close(client);
    return 0;
}

