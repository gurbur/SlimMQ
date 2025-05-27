#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "../include/event_queue.h"

void test_single_push_pop() {
	slimmq_event_queue_t queue;
	event_queue_init(&queue);

	const char* topic = "sensor/room1/temp";
	const char* data = "23.5C";

	int push_result = event_queue_push(&queue, topic, data, strlen(data));
	assert(push_result == 0);

	slimmq_event_t event;
	int pop_result = event_queue_pop(&queue, &event);
	assert(pop_result == 0);

	assert(strcmp(event.topic, topic) == 0);
	assert(memcmp(event.data, data, strlen(data)) == 0);
	assert(event.data_len == strlen(data));

	printf("[PASS] Push/pop single event with topic/data verified.\n");

	free(event.data);
	event_queue_destroy(&queue);
}

void test_fifo_order() {
	slimmq_event_queue_t queue;
	event_queue_init(&queue);

	const char* topics[] = { "a", "b", "c" };
	const char* messages[] = { "1", "2", "3" };

	for (int i = 0; i < 3; ++i) {
		int r = event_queue_push(&queue, topics[i], messages[i], strlen(messages[i]));
		assert(r == 0);
	}

	for (int i = 0; i < 3; ++i) {
		slimmq_event_t ev;
		assert(event_queue_pop(&queue, &ev) == 0);
		assert(strcmp(ev.topic, topics[i]) == 0);
		assert(memcmp(ev.data, messages[i], strlen(messages[i])) == 0);
		free(ev.data);
	}

	printf("[PASS] FIFO order maintained across multiple push/pop.\n");
	event_queue_destroy(&queue);
}

void test_event_queue_overflow() {
	slimmq_event_queue_t queue;
	event_queue_init(&queue);

	const char* topic = "overflow/test";
	const char* payload = "data";

	for (int i = 0; i < MAX_EVENT_QUEUE_SIZE; ++i) {
		int r = event_queue_push(&queue, topic, payload, strlen(payload));
		assert(r == 0);
	}

	int overflow_result = event_queue_push(&queue, topic, payload, strlen(payload));
	assert(overflow_result == -1);

	printf("[PASS] Overflow test: queue rejected extra push after max capacity.\n");

	for (int i = 0; i < MAX_EVENT_QUEUE_SIZE; ++i) {
		slimmq_event_t e;
		assert(event_queue_pop(&queue, &e) == 0);
		free(e.data);
	}

	event_queue_destroy(&queue);
}

int main() {
	test_single_push_pop();
	test_fifo_order();
	test_event_queue_overflow();

	printf("[PASS] All event_queue tests passed\n");
	return 0;
}
