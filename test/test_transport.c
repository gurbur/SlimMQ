#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../include/transport.h"
#include "../include/nano_msg.h"
#include "../include/packet_handler.h"

const char* test_payload = "hello from nanoBUS!";
const size_t test_payload_len = 20;

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
	nano_msg_header_t header = {
		.version = 1,
		.msg_type = MSG_DATA,
		.qos_level = QOS_AT_MOST_ONCE,
		.topic_id = 1001,
		.msg_id = 777,
		.frag_id = 0,
		.frag_total = 1,
		.batch_size = 1,
		.payload_length = test_payload_len,
		.client_node_count = 1
	};

	// 4. send message
	int bytes_sent = send_message(sockfd, (struct sockaddr*)&dest_addr, addrlen, &header, test_payload);
	assert(bytes_sent > 0);

	// 5. receive message
	nano_msg_header_t recv_header;
	char recv_payload[256];
	struct sockaddr_in recv_from;
	socklen_t from_len = sizeof(recv_from);

	int recv_status = recv_message(sockfd, &recv_header, recv_payload, sizeof(recv_payload), (struct sockaddr*)&recv_from, &from_len);
	assert(recv_status == 0);

	// 6. compare received message
	assert(memcmp(&header, &recv_header, sizeof(nano_msg_header_t)) == 0);
	assert(memcmp(test_payload, recv_payload, test_payload_len) == 0);

	printf("[PASS] UDP transport loopback test successful.\n");

	close(sockfd);
}

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
			enable_transport_debug(true);
		}
	}

	test_udp_loopback();
	return 0;
}
