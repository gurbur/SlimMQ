#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../include/slimmq_client.h"
#include "../include/slim_msg.h"
#include "../include/transport.h"
#include "../include/packet_handler.h"
#include "test_common.h"

#define BROKER_PORT 9900
#define BROKER_IP "127.0.0.1"

volatile int mock_broker_ready = 0;

void* mock_broker_thread(void* arg) {
	int sockfd = init_udp_socket(BROKER_IP, BROKER_PORT);
	assert(sockfd >= 0);

	mock_broker_ready = 1;

	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	uint8_t buf[1024];
	slim_msg_header_t header;
	char topic[128];
	char payload[512];

	int len = recv_bytes(sockfd, buf, sizeof(buf),
												(struct sockaddr*)&client_addr,
												&addrlen);
	assert(len > 0);
	deserialize_message(buf, len, &header,
											topic, sizeof(topic),
											payload, sizeof(payload));

	ASSERT_EQ(header.msg_type, MSG_SUBSCRIBE);
	ASSERT_EQ(strcmp(topic, "test/topic"), 0);
	
	len = recv_bytes(sockfd, buf, sizeof(buf), (struct sockaddr*)&client_addr,
									&addrlen);
	assert(len > 0);
	deserialize_message(buf, len, &header,
											topic, sizeof(topic),
											payload, sizeof(payload));
	ASSERT_EQ(header.msg_type, MSG_PUBLISH);
	ASSERT_EQ(strcmp(topic, "test/topic"), 0);
	ASSERT_EQ(strcmp(payload, "hello"), 0);

	int resp_len = serialize_message(&header, topic, payload,
																	strlen(payload), buf,
																	sizeof(buf));
	send_bytes(sockfd, (struct sockaddr*)&client_addr, addrlen,
						buf, resp_len);

	close(sockfd);
	return NULL;
}

void test_slimmq_client_publish_subscribe() {
	pthread_t broker_thread;
	pthread_create(&broker_thread, NULL, mock_broker_thread, NULL);

	while(!mock_broker_ready) usleep(10000);

	slimmq_client_t* client = slimmq_connect(BROKER_IP, BROKER_PORT);
	assert(client != NULL);

	int sub = slimmq_subscribe(client, "test/topic");
	ASSERT_EQ(sub >= 0, true);

	int pub = slimmq_publish(client, "test/topic", "hello", strlen("hello"));
	ASSERT_EQ(pub >= 0, true);

	char recv_topic[128];
	char recv_data[512];
	int res = slimmq_receive(client, recv_topic, sizeof(recv_topic), recv_data, sizeof(recv_data));
	ASSERT_EQ(res, 0);
	ASSERT_EQ(strcmp(recv_topic, "test/topic"), 0);
	ASSERT_EQ(strcmp(recv_data, "hello"), 0);

	slimmq_close(client);
	pthread_join(broker_thread, NULL);
}

int main() {
	RUN_TEST(test_slimmq_client_publish_subscribe);
	return 0;
}

