#pragma once
#include <stdint.h>

#define MSG_DATA 0
#define MSG_ACK 1
#define MSG_NODE_COUNT_UPDATE 2
#define MSG_CONTROL 3

#define QOS_AT_MOST_ONCE 0
#define QOS_AT_LEAST_ONCE 1
#define QOS_EXACTLY_ONCE 2

#pragma pack(push, 1)
typedef struct {
	uint8_t version;
	uint8_t msg_type;
	uint8_t qos_level;
	uint16_t topic_id;
	uint32_t msg_id;
	uint8_t frag_id;
	uint8_t frag_total;
	uint8_t batch_size;
	uint16_t payload_length;
	uint8_t client_node_count;
} nano_msg_header_t;
#pragma pack(pop)

int serialize_message(const nano_msg_header_t* header, const void* payload, uint8_t* out_buf, size_t buf_size);

int deserialize_message(const uint8_t* in_buf, size_t buf_len, nano_msg_header_t* out_header, void* out_payload, size_t max_payload);

