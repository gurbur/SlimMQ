#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/event_queue.h"
#include "../include/slim_msg.h"

void event_queue_init(slimmq_event_queue_t* q) {
	memset(q, 0, sizeof(*q));
	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->not_empty, NULL);
}

void event_queue_destroy(slimmq_event_queue_t *q) {
	pthread_mutex_destroy(&q->lock);
	pthread_cond_destroy(&q->not_empty);

	for (size_t i = 0; i < q->count; ++i) {
		size_t idx = (q->head + i) % MAX_EVENT_QUEUE_SIZE;
		free(q->buffer[idx].data);
	}
}

int event_queue_push(slimmq_event_queue_t *q, uint8_t msg_type, uint32_t msg_id,
										const char *topic, const void *data, size_t len) {
	pthread_mutex_lock(&q->lock);

	if (q->count >= MAX_EVENT_QUEUE_SIZE) {
		pthread_mutex_unlock(&q->lock);
		return -1;
	}

	size_t idx = q->tail;

	q->buffer[idx].msg_type = msg_type;
	q->buffer[idx].msg_id = msg_id;

	strncpy(q->buffer[idx].topic, topic, MAX_TOPIC_LEN - 1);
	q->buffer[idx].topic[MAX_TOPIC_LEN - 1] = '\0';

	q->buffer[idx].data = NULL;
	q->buffer[idx].data_len = 0;
	if (len > 0) {
		q->buffer[idx].data = malloc(len);
		if (!q->buffer[idx].data) {
			pthread_mutex_unlock(&q->lock);
			return -2;
		}
		memcpy(q->buffer[idx].data, data, len);
		q->buffer[idx].data_len = len;
	}

	q->tail = (q->tail + 1) % MAX_EVENT_QUEUE_SIZE;
	q->count++;

	pthread_cond_signal(&q->not_empty);
	pthread_mutex_unlock(&q->lock);
	return 0;
}

int event_queue_pop(slimmq_event_queue_t *q, slimmq_event_t *out_event) {
	pthread_mutex_lock(&q->lock);

	while(q->count == 0) {
		pthread_cond_wait(&q->not_empty, &q->lock);
	}

	size_t idx = q->head;
	*out_event = q->buffer[idx];

	q->head = (q->head + 1) % MAX_EVENT_QUEUE_SIZE;
	q->count--;
	
	pthread_mutex_unlock(&q->lock);
	return 0;
}

int event_queue_wait_ack(slimmq_event_queue_t* q, uint32_t expected_msg_id) {
	pthread_mutex_lock(&q->lock);

	for (size_t i = 0; i < q->count; ++i) {
		size_t idx = (q->head + i) % MAX_EVENT_QUEUE_SIZE;
		slimmq_event_t* evt = &q->buffer[idx];

		if (evt->msg_type == MSG_ACK && evt->msg_id == expected_msg_id) {
			free(evt->data);

			for (size_t j = i; j < q->count - 1; ++j) {
				size_t from = (q->head + j + 1) % MAX_EVENT_QUEUE_SIZE;
				size_t to = (q->head + j) % MAX_EVENT_QUEUE_SIZE;
				q->buffer[to] = q->buffer[from];
			}

			q->tail = (q->tail + MAX_EVENT_QUEUE_SIZE - 1) % MAX_EVENT_QUEUE_SIZE;
			q->count--;

			pthread_mutex_unlock(&q->lock);
			return 0;
		}
	}
	pthread_mutex_unlock(&q->lock);
	return -1;
}
