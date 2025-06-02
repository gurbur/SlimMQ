#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/pending_table.h"

#define PENDING_TABLE_SIZE 1024

static pending_entry_t* table[PENDING_TABLE_SIZE];

static uint32_t hash_key(const struct sockaddr_in* addr, uint32_t msg_id) {
	return ((addr->sin_addr.s_addr ^ addr->sin_port) ^ msg_id) % PENDING_TABLE_SIZE;
}

void pending_table_init(void) {
	memset(table, 0, sizeof(table));
}

void pending_table_destroy() {
	for (int i = 0; i < PENDING_TABLE_SIZE; ++i) {
		pending_entry_t* cur = table[i];
		while (cur) {
			pending_entry_t* next = cur->next;
			free(cur);
			cur = next;
		}
		table[i] = NULL;
	}
}

void pending_table_update(const struct sockaddr_in *addr, uint32_t msg_id, qos2_state_t state) {
	uint32_t idx = hash_key(addr, msg_id);
	pending_entry_t* cur = table[idx];
	time_t now = time(NULL);

	while(cur) {
		if (cur->msg_id == msg_id && memcmp(&cur->addr, addr, sizeof(struct sockaddr_in)) == 0) {
			cur->state = state;
			cur->timestamp = now;
			return;
		}
		cur = cur->next;
	}

	pending_entry_t* new_entry = calloc(1, sizeof(pending_entry_t));
	new_entry->addr = *addr;
	new_entry->msg_id = msg_id;
	new_entry->state = state;
	new_entry->timestamp = now;
	new_entry->next = table[idx];
	table[idx] = new_entry;
}

bool pending_table_get(const struct sockaddr_in *addr, uint32_t msg_id, qos2_state_t *out_state) {
	uint32_t idx = hash_key(addr, msg_id);
	pending_entry_t* cur = table[idx];

	while (cur) {
		if (cur->msg_id == msg_id && memcmp(&cur->addr, addr, sizeof(struct sockaddr_in)) == 0) {
			if (out_state) *out_state = cur->state;
			return true;
		}
		cur = cur->next;
	}
	return false;
}

void pending_table_remove(const struct sockaddr_in *addr, uint32_t msg_id) {
	uint32_t idx = hash_key(addr, msg_id);
	pending_entry_t* cur = table[idx];
	pending_entry_t* prev = NULL;

	while (cur) {
		if (cur->msg_id == msg_id && memcmp(&cur->addr, addr, sizeof(struct sockaddr_in)) == 0) {
			if (prev) prev->next = cur->next;
			else table[idx] = cur->next;
			free(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

void pending_table_cleanup_expired(time_t expiration_sec) {
	time_t now = time(NULL);
	for (int i = 0; i < PENDING_TABLE_SIZE; ++i) {
		pending_entry_t* cur = table[i];
		pending_entry_t* prev = NULL;

		while (cur) {
			if (now - cur->timestamp > expiration_sec) {
				pending_entry_t* to_delete = cur;
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
