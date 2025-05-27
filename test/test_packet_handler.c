#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"

void test_serialization_deserialization() {
    slim_msg_header_t header = {
        .version = 1,
        .msg_type = MSG_PUBLISH,
        .qos_level = QOS_AT_MOST_ONCE,
        .topic_id = 0,
        .msg_id = 42,
        .frag_id = 0,
        .frag_total = 1,
        .batch_size = 1,
        .client_node_count = 1
    };

    const char* topic = "sensor/temp/room1";
    const char* message = "value=22.5";
    size_t msg_len = strlen(message);
    size_t topic_len = strlen(topic);

    // Set payload_length correctly
    header.payload_length = 1 + topic_len + msg_len;

    uint8_t buffer[512];
    int serialized_len = serialize_message(&header, topic,
																					message, msg_len,
																					buffer,
																					sizeof(buffer));
    assert(serialized_len > 0);
		assert(serialized_len == sizeof(slim_msg_header_t) + header.payload_length);

    slim_msg_header_t parsed_header;
    char parsed_topic[128] = {0};
    char parsed_message[256] = {0};

    int status = deserialize_message(buffer, serialized_len,
                                     &parsed_header,
																		 parsed_topic,
																		 sizeof(parsed_topic),
                                     parsed_message,
																		 sizeof(parsed_message));
    assert(status == 0);

    assert(parsed_header.version == header.version);
    assert(parsed_header.msg_type == header.msg_type);
    assert(parsed_header.msg_id == header.msg_id);
		assert(parsed_header.payload_length == header.payload_length);

    assert(strcmp(parsed_topic, topic) == 0);

    assert(strncmp(parsed_message, message, msg_len) == 0);
		assert(parsed_message[msg_len] == '\0');

    printf("[PASS] serialize/deserialize with topic+message structure works correctly.\n");
}

int main() {
    test_serialization_deserialization();
    return 0;
}

