#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "test_common.h"
#include "../include/transport.h"
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"

const char* test_payload = "hello from SlimMQ!";
const size_t test_payload_len = 20;
const char* test_topic = "test/loopback";

void test_udp_loopback() {
	// 1. create socket & binding (loopback)
	int sockfd = init_udp_socket("127.0.0.1", 9000);
	ASSERT_TRUE(sockfd >= 0);

	// 2. set destination address (loopback)
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(9000);
	inet_pton(AF_INET, "127.0.0.1", &dest_addr.sin_addr);

	// 3. compose message header
	slim_msg_header_t header = {
		.version = 1,
		.msg_type = MSG_PUBLISH,
		.qos_level = QOS_AT_MOST_ONCE,
		.topic_id = 1001,
		.msg_id = 777,
		.frag_id = 0,
		.frag_total = 1,
		.batch_size = 1,
		.payload_length = 1 + strlen(test_topic) + test_payload_len,
		.client_node_count = 1
	};

	// 4. serialize send message
	uint8_t out_buf[512];
	int bytes_sent = serialize_message(&header, test_topic,
																		test_payload,
																		test_payload_len, out_buf,
																		sizeof(out_buf));
	ASSERT_TRUE(bytes_sent > 0);

	int sent = send_bytes(sockfd, (struct sockaddr*)&dest_addr,
												sizeof(dest_addr), out_buf, bytes_sent);
	ASSERT_EQ(sent, bytes_sent);

	// 5. receive message
	struct sockaddr_in recv_from;
	socklen_t from_len = sizeof(recv_from);
	uint8_t in_buf[512];
	int received = recv_bytes(sockfd, in_buf, sizeof(in_buf),
													(struct sockaddr*)&recv_from,
													&from_len);
	ASSERT_TRUE(received > 0);

	// 6. deserialize
	slim_msg_header_t recv_header;
	char recv_topic[128];
	char recv_payload[256];

	int status = deserialize_message(in_buf, received,
																	&recv_header,
																	recv_topic,
																	sizeof(recv_topic),
																	recv_payload,
																	sizeof(recv_payload));

	ASSERT_EQ(status, 0);

	dump_header(&recv_header);
	dump_payload(recv_payload, recv_header.payload_length);

	// 7. assert correctness
	ASSERT_STR_EQ(recv_topic, test_topic);
	ASSERT_TRUE(memcmp(test_payload, recv_payload, test_payload_len) == 0);

	close(sockfd);
}

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
			enable_transport_debug(true);
			set_packet_debug(true);
		}
	}

	RUN_TEST(test_udp_loopback);
	return 0;
}
