#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/topic_table.h"

typedef struct subscriber_list_entry {
	struct sockaddr_in addr;
	struct subscriber_list_entry* next;
} subscriber_list_entry;

typedef struct topic_node {
	char* segment;
	struct topic_node** children;
	size_t child_count;

	subscriber_list_entry* subscribers;
} topic_node;

static topic_node* topic_root = NULL;

/**
 * remove_duplicates - 
 */
void remove_duplicates(SubscriberList* list) {
	Subscriber* prev = NULL;
	Subscriber* curr = list->head;

	while(curr) {
		Subscriber* runner = list->head;
		bool found_duplicate = false;

		while(runner != curr) {
			if (memcmp(&runner->addr, &curr->addr, sizeof(struct sockaddr_in)) == 0) {
				found_duplicate = true;
				break;
			}
			runner = runner->next;
		}

		if (found_duplicate) {
			Subscriber* temp = curr;
			curr = curr->next;
			if (prev) prev->next = curr;
			else list->head = curr;
			free(temp);
			list->count--;
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
}

/**
 * is_in_list - Check if given address is in subscriber list
 *
 * @list: subscriber list to check
 * @addr: finding address
 *
 * Return: true if in list, false otherwise
 */
static bool is_in_list(SubscriberList* list, const struct sockaddr_in* addr) {
	for (Subscriber* s = list->head; s != NULL; s = s->next) {
		if (memcmp(&s->addr, addr, sizeof(struct sockaddr_in)) == 0) {
			return true;
		}
	}
	return false;
}

/**
 * is_duplicate_subscriber - Check if the subscriber already exists in the list
 *
 * @list: subscriber_list_entry* (linked list of subscribers)
 * @addr: address of the subscriber to check
 *
 * Return: true if duplicate, false otherwise
 */
static bool is_duplicate_subscriber(subscriber_list_entry* list, const struct sockaddr_in* addr) {
	for (; list != NULL; list = list->next) {
		if (memcmp(&list->addr, addr, sizeof(struct sockaddr_in)) == 0) {
			return true;
		}
	}
	return false;
}

/**
 * init_topic_table - initialize topic table
 */
void init_topic_table(void) {
	topic_root = calloc(1, sizeof(topic_node));
	topic_root->segment = strdup("");
}

/**
 * free_subscriber_list - free subscriber list
 *
 * @list: list returned from get_matching_subscribers()
 */
void free_subscriber_list(SubscriberList* list) {
	if (!list) return;
	Subscriber* cur = list->head;
	while(cur) {
		Subscriber* next = cur->next;
		free(cur);
		cur = next;
	}
	free(list);
}

/**
 * add_subscriber - add subscriber node to list internally
 */
static void add_subscriber(subscriber_list_entry** list, const struct sockaddr_in* addr) {
	subscriber_list_entry* node = calloc(1, sizeof(subscriber_list_entry));
	memcpy(&node->addr, addr, sizeof(struct sockaddr_in));
	node->next = *list;
	*list = node;
}


/**
 * find_or_create_child - return or create child node that has given segment
 *
 * @parent: upper node
 * @segment: segment string
 *
 * Return: child node that has given segment
 */
static topic_node* find_or_create_child(topic_node* parent, const char* segment) {
	for(size_t i = 0; i < parent->child_count; ++i) {
		if (strcmp(parent->children[i]->segment, segment) == 0) {
			return parent->children[i];
		}
	}

	topic_node* child = calloc(1, sizeof(topic_node));
	child->segment = strdup(segment);
	parent->children = realloc(parent->children, sizeof(topic_node*) * (parent->child_count + 1));
	parent->children[parent->child_count++] = child;
	return child;
}

/**
 * split_topic - split topic string in token '/' and return them
 *
 * @topic: input string
 * @count_out: pointer of divided segment number
 *
 * Return: segment string array (need to be freed when end)
 */
static char** split_topic(const char* topic, int* count_out) {
	char* topic_copy = strdup(topic);
	int count = 0;
	for (char* p = topic_copy; *p; ++p) {
		if (*p == '/') count++;
	}
	count++;

	char** segments = calloc(count, sizeof(char*));
	char* token = strtok(topic_copy, "/");
	int i = 0;
	while(token) {
		segments[i++] = strdup(token);
		token = strtok(NULL, "/");
	}
	*count_out = count;
	free(topic_copy);

	return segments;
}

/**
 * subscribe_topic - register subscriber to MQTT styled topic
 *
 * @topic_str: string of subscribe topic (ex: "sensor/+/temp")
 * @addr: address information of subscriber
 *
 * Return: 0 on success
 */
int subscribe_topic(const char* topic_str, const struct sockaddr_in* addr) {
	int depth = 0;
	char** segments = split_topic(topic_str, &depth);

	topic_node* curr = topic_root;
	for(int i = 0; i < depth; ++i) {
		curr = find_or_create_child(curr, segments[i]);
	}

	if (!is_duplicate_subscriber(curr->subscribers, addr)) {
		add_subscriber(&curr->subscribers, addr);
	}

	for(int i = 0; i < depth; ++i) free(segments[i]);
	free(segments);
	return 0;
}

/**
 * match_recursive - search every subscriber in given topic path
 *
 * @node: currently searching node
 * @segments: segment array of topic path
 * @depth: segment count
 * @level: current searching level
 * @result: result for subscriber list
 */
static void match_recursive(topic_node* node, char** segments, int depth, int level, SubscriberList* result) {
	if (!node) return;
	if (level == depth || strcmp(node->segment, "#") == 0) {
		subscriber_list_entry* s = node->subscribers;
		while(s) {
			if (!is_in_list(result, &s->addr)) {
				Subscriber* copy = calloc(1, sizeof(Subscriber));
				memcpy(&copy->addr, &s->addr, sizeof(struct sockaddr_in));
				copy->next = result->head;
				result->head = copy;
				result->count++;
			}
			s = s->next;
		}
	}

	if (level >= depth) return;

	for (size_t i = 0; i < node->child_count; ++i) {
		topic_node* child = node->children[i];
		if (strcmp(child->segment, segments[level]) == 0 ||
		    strcmp(child->segment, "+") == 0 ||
		    strcmp(child->segment, "#") == 0) {
			match_recursive(child, segments, depth, level + 1, result);
		}
	}
}

/**
 * get_matching_subscribers - return subscribers list matching for published topic
 *
 * @topic_str: topic string of published message
 *
 * Return: pointer of SubscriberList(free after use, using free_subscriber_list())
 */
SubscriberList* get_matching_subscribers(const char* topic_str) {
	int depth = 0;
	char** segments = split_topic(topic_str, &depth);

	SubscriberList* list = calloc(1, sizeof(SubscriberList));
	match_recursive(topic_root, segments, depth, 0, list);

	for (int i = 0; i < depth; ++i) free(segments[i]);
	free(segments);

	remove_duplicates(list);

	return list;
}

/**
 * free_topic_node - free topic node, its children, and its subscribers recursively
 *
 * @node: current freeing node
 */
static void free_topic_node(topic_node* node) {
	if (!node) return;

	subscriber_list_entry* sub = node->subscribers;
	while(sub) {
		subscriber_list_entry* next = sub->next;
		free(sub);
		sub = next;
	}

	for (size_t i = 0; i < node->child_count; ++i) {
		free_topic_node(node->children[i]);
	}

	free(node->segment);
	free(node->children);
	free(node);
}

/**
 * free_tpic_table - free every topic table
 */
void free_topic_table(void) {
	free_topic_node(topic_root);
	topic_root = NULL;
}

/**
 * print_topic_tree_recursive - print topic tree node in depth
 *
 * @node: currently searching node
 * @depth: current depth(root is 0)
 */
static void print_topic_tree_recursive(topic_node* node, int depth) {
	if (!node) return;

	for (int i = 0; i < depth; i++) {
		printf("   ");
	}

	printf("- %s", node->segment);
	if (node->subscribers) {
		printf(" (%s)", node->subscribers ? "subscribers" : "");
	}
	printf("\n");

	for (size_t i = 0; i < node->child_count; ++i) {
		print_topic_tree_recursive(node->children[i], depth + 1);
	}
}

/**
 * print_topic_tree - print topic tree
 */
void print_topic_tree(void) {
	printf("== topic tree ==\n");
	print_topic_tree_recursive(topic_root, 0);
	printf("================\n");
}

