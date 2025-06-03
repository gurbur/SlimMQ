#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define QOS2_CLIENT_TABLE_SIZE 256

typedef enum {
	QOS2_CLIENT_STATE_NONE = 0,
	QOS2_CLIENT_STATE_WAIT_RECEIVED,
	QOS2_CLIENT_STATE_WAIT_COMPLETE,
	QOS2_CLIENT_STATE_COMPLETED
} qos2_client_state_t;

typedef struct qos2_client_entry {
	uint32_t msg_id;
	qos2_client_state_t state;
	time_t timestamp;
	struct qos2_client_entry* next;
} qos2_client_entry_t;

void qos2_client_table_init(void);
void qos2_client_table_destroy(void);
void qos2_client_table_set(uint32_t msg_id, qos2_client_state_t state);
qos2_client_state_t qos2_client_table_get(uint32_t msg_id);
void qos2_client_table_remove(uint32_t msg_id);
void qos2_client_table_cleanup_expired(time_t expiration_sec);

