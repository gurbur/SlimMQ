#include <string.h>
#include <stdlib.h>
#include "test_common.h"
#include "../include/event_queue.h"

void test_single_push_pop() {
	slimmq_event_queue_t queue;
	event_queue_init(&queue);

	const char* topic = "sensor/room1/temp";
	const char* data = "23.5C";

	ASSERT_EQ(event_queue_push(&queue, topic, data, strlen(data)), 0);

	slimmq_event_t event;
	ASSERT_EQ(event_queue_pop(&queue, &event), 0);

	ASSERT_STR_EQ(event.topic, topic);
	ASSERT_EQ(event.data_len, strlen(data));
	ASSERT_TRUE(memcmp(event.data, data, event.data_len) == 0);

	free(event.data);
	event_queue_destroy(&queue);
}

void test_fifo_order() {
	slimmq_event_queue_t queue;
	event_queue_init(&queue);

	const char* topics[] = { "a", "b", "c" };
	const char* messages[] = { "1", "2", "3" };

	for (int i = 0; i < 3; ++i) {
		ASSERT_EQ(event_queue_push(&queue, topics[i], messages[i], strlen(messages[i])), 0);
	}

	for (int i = 0; i < 3; ++i) {
		slimmq_event_t e;
		ASSERT_EQ(event_queue_pop(&queue, &e), 0);
		ASSERT_STR_EQ(e.topic, topics[i]);
		ASSERT_TRUE(memcmp(e.data, messages[i], strlen(messages[i])) == 0);
		free(e.data);
	}

	event_queue_destroy(&queue);
}

void test_event_queue_overflow() {
	slimmq_event_queue_t queue;
	event_queue_init(&queue);

	const char* topic = "overflow/test";
	const char* payload = "data";

	for (int i = 0; i < MAX_EVENT_QUEUE_SIZE; ++i) {
		ASSERT_EQ(event_queue_push(&queue, topic, payload, strlen(payload)), 0);
	}

	int result = event_queue_push(&queue, topic, payload, strlen(payload));
	ASSERT_EQ(result, -1);

	for (int i = 0; i < MAX_EVENT_QUEUE_SIZE; ++i) {
		slimmq_event_t e;
		ASSERT_EQ(event_queue_pop(&queue, &e), 0);
		free(e.data);
	}

	event_queue_destroy(&queue);
}

int main() {
	RUN_TEST(test_single_push_pop);
	RUN_TEST(test_fifo_order);
	RUN_TEST(test_event_queue_overflow);

	printf("=== All event_queue tests passed ===\n");
	return 0;
}
