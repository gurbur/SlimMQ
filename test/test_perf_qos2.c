#include <stdio.h>
#include <string.h>
#include "../include/slimmq_client.h"
#include "../include/slim_msg.h"

#define COUNT 1000

int main() {
    slimmq_client_t* client = slimmq_connect("127.0.0.1", 9000);
    if (!client) {
        fprintf(stderr, "Failed to connect to broker\n");
        return 1;
    }

    slimmq_set_qos(client, QOS_EXACTLY_ONCE);
    slimmq_set_retry_policy(client, 1000, 5);

    for (int i = 0; i < COUNT; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "qos2-message-%d", i);
        slimmq_publish(client, "test/perf", msg, strlen(msg));
    }

    printf("QoS 2: Sent %d messages with exactly-once delivery\n", COUNT);
    slimmq_close(client);
    return 0;
}

