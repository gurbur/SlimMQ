#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/slimmq_client.h"
#include "../include/slim_msg.h"

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

    slimmq_set_qos(client, QOS_AT_LEAST_ONCE);
    slimmq_set_retry_policy(client, 1000, 5);  // 최대 5번 재전송

    for (int i = 0; i < 100; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "msg-%03d", i);
        slimmq_publish(client, "loss/qos1", msg, strlen(msg));
    }

    printf("QoS1: Sent 100 messages with retry policy\n");
    slimmq_close(client);
    return 0;
}

