#define MAX_BUFFER_SIZE 256

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "../include/nano_msg.h"

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

void dump_hex(const uint8_t* data, size_t len) {
	if (!dumping_enabled) return;

	printf("[DUMP]Raw buffer (%zu bytes): ", len);
	for (size_t i = 0; i < len; ++i) {
		printf("%02X ", data[i]);
	}
	printf("\n");
}

void dump_header(const nano_msg_header_t* hdr) {
	if (!dumping_enabled) return;

	printf("[HEADER] version=%u, msg_type=%u, qos=%u, topic_id=%u, msg_id=%u\n", 
		hdr->version, hdr->msg_type, hdr->qos_level,
		hdr->topic_id, hdr->msg_id);
	printf("	 frag_id=%u/%u, batch_size=%u, payload_len=%u, client_node_count=%u\n",
		hdr->frag_id, hdr->frag_total,
		hdr->batch_size, hdr->payload_length, hdr->client_node_count);
}

void test_serialization_and_deserialization() {
	uint8_t buffer[MAX_BUFFER_SIZE];
	int serialized_len = serialize_message(&header, test_payload, buffer, sizeof(buffer));
	assert(serialized_len > 0);

	dump_hex(buffer, serialized_len);

	nano_msg_header_t out_header;
	char out_payload[256];
	int result = deserialize_message(buffer, serialized_len, &out_header, out_payload, sizeof(out_payload));
	assert(result == 0);

	dump_header(&out_header);

	assert(memcmp(&header, &out_header, sizeof(nano_msg_header_t)) == 0);
	assert(memcmp(test_payload, out_payload, test_payload_len) == 0);

	printf("[PASS] Serialization and deserialization test successful.\n");
}

int main(int argc, char* argv[]) {
	for(int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
			dumping_enabled = true;
		}
	}

	test_serialization_and_deserialization();
	return 0;
}

