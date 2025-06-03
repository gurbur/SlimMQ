#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <netinet/in.h>

#define QOS1_BROKER_TABLE_SIZE 512
#define QOS1_MAX_TOPIC_LEN 128
#define QOS1_MAX_PAYLOAD_LEN 1024
#define QOS1_MAX_RETRIES 10

typedef struct qos1_broker_entry {
	struct sockaddr_in addr;
	uint32_t msg_id;
	char topic[QOS1_MAX_TOPIC_LEN];
	uint8_t payload[QOS1_MAX_PAYLOAD_LEN];
	size_t payload_len;
	int retry_count;
	bool active;
	time_t timestamp;
	struct qos1_broker_entry* next;
} qos1_broker_entry_t;

void qos1_broker_table_init(void);
void qos1_broker_table_destroy(void);

void qos1_broker_table_register(uint32_t msg_id, const char* topic,
																const uint8_t* payload, size_t payload_len,
																const struct sockaddr_in* publisher_addr);

void qos1_broker_table_acknowledge(uint32_t msg_id);

bool qos1_broker_table_has_pending(void);

void qos1_broker_table_resend_pending(int sockfd);
