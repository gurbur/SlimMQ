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

