#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/qos2_table.h"

static qos2_entry_t* table[QOS2_TABLE_SIZE];

static uint32_t hash_msg_id(uint32_t msg_id) {
	return msg_id % QOS2_TABLE_SIZE;
}

void qos2_table_init(void) {
	memset(table, 0, sizeof(table));
}

void qos2_table_destroy(void) {
	for (int i = 0; i < QOS2_TABLE_SIZE; ++i) {
		qos2_entry_t* cur = table[i];
		while(cur) {
			qos2_entry_t* next = cur->next;
			free(cur);
			cur = next;
		}
		table[i] = NULL;
	}
}

void qos2_table_set(uint32_t msg_id, qos2_client_state_t state) {
	uint32_t idx = hash_msg_id(msg_id);
	qos2_entry_t* cur = table[idx];
	time_t now = time(NULL);

	while (cur) {
		if (cur->msg_id == msg_id) {
			cur->state = state;
			cur->timestamp = now;
			return;
		}
		cur = cur->next;
	}

	qos2_entry_t* new_entry = calloc(1, sizeof(qos2_entry_t));
	new_entry->msg_id = msg_id;
	new_entry->state = state;
	new_entry->timestamp = now;
	new_entry->next = table[idx];
	table[idx] = new_entry;
}

qos2_client_state_t qos2_table_get_state(uint32_t msg_id) {
	uint32_t idx = hash_msg_id(msg_id);
	qos2_entry_t* cur = table[idx];

	while(cur) {
		if (cur->msg_id == msg_id) {
			return cur->state;
		}
		cur = cur->next;
	}
	return QOS2_CLIENT_STATE_NONE;
}

void qos2_table_remove(uint32_t msg_id) {
	uint32_t idx = hash_msg_id(msg_id);
	qos2_entry_t* cur = table[idx];
	qos2_entry_t* prev = NULL;

	while(cur) {
		if (cur->msg_id == msg_id) {
			if (prev) prev->next = cur->next;
			else table[idx] = cur->next;
			free(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

void qos2_table_cleanup_expired(time_t expiration_sec) {
	time_t now = time(NULL);
	for (int i = 0; i < QOS2_TABLE_SIZE; ++i) {
		qos2_entry_t* cur = table[i];
		qos2_entry_t* prev = NULL;

		while (cur) {
			if (now - cur->timestamp > expiration_sec) {
				qos2_entry_t* to_delete = cur;
				if (prev) prev->next = cur->next;
				else table[i] = cur->next;
				cur = cur->next;
				free(to_delete);
			} else {
				prev = cur;
				cur = cur->next;
			}
		}
	}
}
