#define MAX_BUFFER_SIZE 256

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "../include/nano_msg.h"
#include "../include/packet_handler.h"

const char* test_payload = "Hello, nanoBUS!";
const size_t test_payload_len = 15;

static bool dumping_enabled = false;

nano_msg_header_t header = {
	.version = 1,
	.msg_type = MSG_DATA,
	.qos_level = QOS_AT_LEAST_ONCE,
	.topic_id = 42,
	.msg_id = 12345,
	.frag_id = 0,
	.frag_total = 1,
	.batch_size = 1,
	.payload_length = test_payload_len,
	.client_node_count = 3
};

void test_serialization_and_deserialization() {
	uint8_t buffer[MAX_BUFFER_SIZE];
	int serialized_len = serialize_message(&header, test_payload, buffer, sizeof(buffer));
	assert(serialized_len > 0);

	nano_msg_header_t out_header;
	char out_payload[256];
	int result = deserialize_message(buffer, serialized_len, &out_header, out_payload, sizeof(out_payload));
	assert(result == 0);

	dump_hex(buffer, serialized_len);
	dump_header(&out_header);
	dump_payload(out_payload, out_header.payload_length);

	assert(memcmp(&header, &out_header, sizeof(nano_msg_header_t)) == 0);
	assert(memcmp(test_payload, out_payload, test_payload_len) == 0);

	printf("[PASS] Serialization and deserialization test successful.\n");
}

int main(int argc, char* argv[]) {
	for(int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
			set_packet_debug(true);
		}
	}

	test_serialization_and_deserialization();
	return 0;
}

