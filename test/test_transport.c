#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../include/transport.h"
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"

const char* test_payload = "hello from SlimMQ!";
const size_t test_payload_len = 20;
const char* test_topic = "test/loopback";

void test_udp_loopback() {
	// 1. create socket & binding (loopback)
	int sockfd = init_udp_socket("127.0.0.1", 9000);
	assert(sockfd >= 0);

	// 2. set destination address (loopback)
	struct sockaddr_in dest_addr;
	socklen_t addrlen = sizeof(dest_addr);
	memset(&dest_addr, 0, addrlen);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(9000);
	inet_pton(AF_INET, "127.0.0.1", &dest_addr.sin_addr);

	// 3. compose message header
	slim_msg_header_t header = {
		.version = 1,
		.msg_type = MSG_DATA,
		.qos_level = QOS_AT_MOST_ONCE,
		.topic_id = 1001,
		.msg_id = 777,
		.frag_id = 0,
		.frag_total = 1,
		.batch_size = 1,
		.payload_length = 1 + strtlen(test_topic) + test_payload_len,
		.client_node_count = 1
	};

	// 4. send message
	uint8_t out_buf[512];
	int bytes_sent = serialize_message(&header, test_topic, test_payload, test_payload_len, out_buf, sizeof(out_buf));
	assert(bytes_sent > 0);

	int sent = send_bytes(sockfd, (struct sockaddr*)&dest_addr, addrlen, out_buf, bytes_sent);
	assert(sent == bytes_sent);

	// 5. receive message
	uint8_t in_buf[512];
	int received = recv_bytes(sockfd, in_buf, sizeof(in_buf), (struct sockaddr*)&recv_from, &from_len);
	assert(received > );

	slim_msg_header_t recv_header;
	char recv_topic[128];
	char recv_payload[256];

	int status = deserialize_message(in_buf, received, &recv_header, recv_topic, sizeof(recv_topic), recv_message, sizeof(recv_message));

	assert(status == 0);

	dump_header(&recv_header);
	dump_payload(recv_payload, recv_header.payload_length);

	// 6. compare received message
	assert(strcmp(recv_topic, test_topic) == 0);
	assert(memcmp(test_payload, recv_payload, test_payload_len) == 0);

	printf("[PASS] UDP transport loopback test successful.\n");

	close(sockfd);
}

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
			enable_transport_debug(true);
			set_packet_debug(true);
		}
	}

	test_udp_loopback();
	return 0;
}
