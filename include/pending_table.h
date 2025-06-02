#pragma once

#include <stdint.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>

typedef enum {
	QOS2_STATE_RECEIVED,
	QOS2_STATE_RELEASED,
	QOS2_STATE_COMPLETED,
} qos2_state_t;

typedef struct pending_entry {
	struct sockaddr_in addr;
	uint32_t msg_id;
	qos2_state_t state;
	time_t timestamp;
	struct pending_entry* next;
} pending_entry_t;

void pending_table_init(void);

void pending_table_destroy(void);

void pending_table_update(const struct sockaddr_in* addr, uint32_t msg_id, qos2_state_t state);

bool pending_table_get(const struct sockaddr_in* addr, uint32_t msg_id, qos2_state_t* out_state);

void pending_table_remove(const struct sockaddr_in* addr, uint32_t msg_id);

void pending_table_cleanup_expired(time_t expiration_sec);

