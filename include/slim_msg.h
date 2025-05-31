#pragma once
#include <stdint.h>

#define MSG_PUBLISH 						0
#define MSG_ACK 								1
#define MSG_NODE_COUNT_UPDATE 	2
#define MSG_CONTROL 						3
#define MSG_SUBSCRIBE 					4

#define QOS_AT_MOST_ONCE 				0
#define QOS_AT_LEAST_ONCE 			1
#define QOS_EXACTLY_ONCE 				2

typedef enum {
	CONTROL_RECEIVED = 0x01, // QoS 2 Step 1 (PUBREC)
	CONTROL_RELEASE  = 0x02, // QoS 2 Step 2 (PUBREL)
	CONTROL_COMPLETE = 0x03  // QoS 2 Step 3 (PUBCOMP)
} control_type_t;

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
} slim_msg_header_t;
#pragma pack(pop)

