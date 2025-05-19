#pragma once

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_TOPIC_LEN 128
#define MAX_EVENT_QUEUE_SIZE 128

typedef struct {
	char topic[MAX_TOPIC_LEN];
	uint8_t* data;
	size_t data_len;
} slimmq_event_t;

typedef struct {
	slimmq_event_t buffer[MAX_EVENT_QUEUE_SIZE];
	size_t head;
	size_t tail;
	size_t count;

	pthread_mutex_t lock;
	pthread_cond_t not_empty;
} slimmq_event_queue_t;

/*
 * event_queue_init: 
 *
 * @q: queue to init
 */
void event_queue_init(slimmq_event_queue_t* q);

/*
 * event_queue_destroy: 
 *
 * @q: queue to destroy
 */
void event_queue_destroy(slimmq_event_queue_t* q);

/*
 * event_queue_push: 
 *
 * @q: queue to push
 * @topic: topic of pushing event
 * @data: data of pushing event
 * @len: length of pushing event
 *
 * Return: 0 on success, -1 on queue full, -2 on queue locked
 */
int event_queue_push(slimmq_event_queue_t* q, const char* topic,
										const void* data, size_t len);

/*
 * event_queue_pop: 
 *
 * @q: queue to pop
 * @out_event: event pointer where popped event go out
 *
 * Return: 0 on success, infinitly looped on no entry
 */
int event_queue_pop(slimmq_event_queue_t* q, slimmq_event_t* out_event);

