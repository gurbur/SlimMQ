#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <netinet/in.h>

#define QOS2_BROKER_TABLE_SIZE 1024

typedef enum {
	QOS2_BROKER_STATE_RECEIVED,
	QOS2_BROKER_STATE_WAIT_RELEASE,
	QOS2_BROKER_STATE_RELEASED,
	QOS2_BROKER_STATE_COMPLETED,
} qos2_broker_state_t;

typedef struct qos2_broker_entry {
	struct sockaddr_in addr;
	uint32_t msg_id;
	qos2_broker_state_t state;
	time_t timestamp;
	struct qos2_broker_entry* next;
} qos2_broker_entry_t;

void qos2_broker_table_init(void);
void qos2_broker_table_destroy(void);
void qos2_broker_table_set(const struct sockaddr_in* addr, uint32_t msg_id, qos2_broker_state_t state);
bool qos2_broker_table_get(const struct sockaddr_in* addr, uint32_t msg_id, qos2_broker_state_t* out_state);
void qos2_broker_table_remove(const struct sockaddr_in* addr, uint32_t msg_id);
void qos2_broker_table_cleanup_expired(time_t expiration_sec);

