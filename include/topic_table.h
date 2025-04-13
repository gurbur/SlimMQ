#pragma once

#include <netinet/in.h>
#include <stddef.h>


typedef struct Subscriber {
	struct sockaddr_in addr;
	struct Subscriber* next;
} Subscriber;

typedef struct SubscriberList {
	Subscriber* head;
	size_t count;
} SubscriberList;

// initialize/destroy topic table
void init_topic_table(void);
void free_topic_table(void);

// add/delete subscriber
int subscribe_topic(const char* topic_str, const struct sockaddr_in* subscriber);
int unsubscribe_topic(const char* topic_str, const struct sockaddr_in* subscriber);

// get subscriber list of given topic
SubscriberList* get_matching_subscribers(const char* topic_str);

// for utilities
void print_topic_tree(void); // for debugging
void free_subscriber_list(SubscriberList* list); // free list

