#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFINE_TABLE_INIT(name, table_size) \
void name##_init(void* table[], size_t size) { \
	memset(table, 0, sizeof(void*) * size); \
}

#define DEFINE_TABLE_DESTROY(name, entry_type, table_size) \
void name##_detroy(void* table[]) { \
	for (int i = 0; i < table_size; ++i) { \
		entry_type* cur = table[i]; \
		while (cur) { \
			entry_type* next = cur->next; \
			free(cur); \
			cur = next; \
		} \
		table[i] = NULL; \
	} \
}

#define DEFINE_TABLE_REMOVE(name, entry_type, match_fn, table_size) \
void name##_remove(void* table[], void* key) { \
	uint32_t idx = match_fn(key); \
	entry_type* cur = table[idx]; \
	entry_type* prev = NULL; \
	while (cur) { \
		if (match_fn(key, cur)) { \
			if (prev) prev->next = cur->next; \
			else table[idx] = cur->next; \
			free(cur); \
			return; \
		} \
		prev = cur; \
		cur = cur->next; \
	} \
}

#define DEFINE_TABLE_CLEANUP_EXPIRED(name, entry_type, table_size) \
void name##_cleanup_expired(void* table[], time_t expiration_sec) { \
	time_t now = time(NULL); \
	for (int i = 0; i < table_size; ++i) { \
		entry_type* cur = table[i]; \
		entry_type* prev = NULL; \
		while (cur) { \
			if (now - cur->timestamp > expiration_sec) { \
				entry_type* to_delete = cur; \
				if (prev) prev->next = cur->next; \
				else table[i] = cur->next; \
				cur = cur->next; \
				free(to_delete); \
			} else { \
				prev = cur; \
				cur = cur->next; \
			} \
		} \
	} \
}

