#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define QOS2_TABLE_SIZE 256

typedef enum {
	QOS2_CLIENT_STATE_NONE = 0,
	QOS2_CLIENT_STATE_WAIT_RECEIVED,
	QOS2_CLIENT_STATE_WAIT_COMPLETE,
	QOS2_CLIENT_STATE_COMPLETED
} qos2_client_state_t;

typedef struct qos2_entry {
	uint32_t msg_id;
	qos2_client_state_t state;
	time_t timestamp;
	struct qos2_entry* next;
} qos2_entry_t;

void qos2_table_init(void);

void qos2_table_destroy(void);

void qos2_table_set(uint32_t msg_id, qos2_client_state_t state);

qos2_client_state_t qos2_table_get_state(uint32_t msg_id);

void qos2_table_remove(uint32_t msg_id);

void qos2_table_cleanup_expired(time_t expiration_sec);

