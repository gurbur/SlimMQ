#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/slimmq_client.h"

#define DEFAULT_BROKER_PORT 9000
#define DEFAULT_BROKER_IP "127.0.0.1"

int main(int argc, char* argv[]) {
	const char* broker_ip = DEFAULT_BROKER_IP;
	int port = DEFAULT_BROKER_PORT;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
			broker_ip = argv[++i];
		} else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
			port = atoi(argv[++i]);
		}
	}
	slimmq_client_t* client = slimmq_connect(broker_ip, port);

    if (!client) return 1;

    slimmq_subscribe(client, "loss/qos1");

    char seen[100] = {0};
    int total = 0, unique = 0;

    while (unique < 100) {
        char topic[128];
        void* data;
        size_t len;
        if (slimmq_next_event(client, topic, sizeof(topic), &data, &len) == 0) {
            int msg_id;
            if (sscanf(data, "msg-%d", &msg_id) == 1 && msg_id < 100) {
                if (!seen[msg_id]) {
                    seen[msg_id] = 1;
                    printf("[QoS1] Received: msg-%03d\n", msg_id);
                    unique++;
                } else {
                    printf("[QoS1] Duplicate: msg-%03d\n", msg_id);
                }
                total++;
            }
            free(data);
        }
    }

    printf("QoS1: Received %d total, %d unique messages\n", total, unique);
    slimmq_close(client);
    return 0;
}

