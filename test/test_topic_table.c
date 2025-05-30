#include <string.h>
#include <arpa/inet.h>
#include "test_common.h"
#include "../include/topic_table.h"

void fill_addr(struct sockaddr_in* addr, const char* ip_str, int port) {
  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  inet_pton(AF_INET, ip_str, &addr->sin_addr);
}

void test_basic_subscribe_and_match() {
	init_topic_table();
	struct sockaddr_in sub1, sub2, sub3;
	fill_addr(&sub1, "127.0.0.1", 10001);
	fill_addr(&sub2, "127.0.0.1", 10002);
	fill_addr(&sub3, "127.0.0.1", 10003);

	subscribe_topic("sensor/temperature/room1", &sub1);
	subscribe_topic("sensor/temperature/+", &sub2);
	subscribe_topic("sensor/#", &sub3);

	SubscriberList* list = get_matching_subscribers("sensor/temperature/room1");
	ASSERT_EQ(list->count, 3);
	free_subscriber_list(list);

	list = get_matching_subscribers("sensor/humidity/room2");
	ASSERT_EQ(list->count, 1);

	print_topic_tree();
	free_subscriber_list(list);
	free_topic_table();
}

void test_multi_topic_match_deduplication() {
	init_topic_table();
	struct sockaddr_in sub;
	fill_addr(&sub, "127.0.0.1", 12345);

	subscribe_topic("sensor/#", &sub);
	subscribe_topic("sensor/temp", &sub);

	SubscriberList* list = get_matching_subscribers("sensor/temp");
	ASSERT_EQ(list->count, 1);
	free_subscriber_list(list);

	subscribe_topic("room/+/temp", &sub);
	subscribe_topic("room/101/temp", &sub);

	SubscriberList* list2 = get_matching_subscribers("room/101/temp");
	ASSERT_EQ(list2->count, 1);
	free_subscriber_list(list2);

	subscribe_topic("building/#", &sub);
	subscribe_topic("building/floor1/room1", &sub);

	SubscriberList* list3 = get_matching_subscribers("building/floor1/room1");
	ASSERT_EQ(list3->count, 1);
	free_subscriber_list(list3);

	print_topic_tree();
	free_topic_table();
}

int main() {

  RUN_TEST(test_basic_subscribe_and_match);
	RUN_TEST(test_multi_topic_match_deduplication);


  return 0;
}

