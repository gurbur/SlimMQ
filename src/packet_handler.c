#include <string.h>
#include <stdio.h>
#include "../include/slim_msg.h"
#include "../include/packet_handler.h"

static bool packet_debug_enabled = false;

/**
 * set_packet_debug - Enable or disable internal hex/header/payload dump
 */
void set_packet_debug(bool enable) {
	packet_debug_enabled = enable;
}

/**
 * dump_hex - Print data into hexadecimal format
 */
void dump_hex(const uint8_t* data, size_t len) {
	if (!packet_debug_enabled || data == NULL) return;

	printf("[HEX] (%zu bytes): ", len);
	for (size_t i = 0; i < len; ++i) {
		printf("%02X ", data[i]);
		if ((i + 1) % 16 == 0) printf("\n");
	}
	if (len % 16 != 0) printf("\n");
}

/**
 * dump_header - Print slim_msg_header_t fields in human-readable format
 */
void dump_header(const slim_msg_header_t* hdr) {
	if (!packet_debug_enabled || hdr == NULL) return;

	printf("[HEADER] version=%u, type=%u, qos=%u, topic_id=%u, msg_id=%u\n", 
		hdr->version, hdr->msg_type, hdr->qos_level,
		hdr->topic_id, hdr->msg_id);
	printf("	 frag_id=%u/%u, batch_size=%u, payload_len=%u, client_nodes=%u\n",
		hdr->frag_id, hdr->frag_total,
		hdr->batch_size, hdr->payload_length, hdr->client_node_count);
}

/**
 * dump_payload - Print payload in hex (16 bytes per line)
 */
void dump_payload(const void* payload, size_t len) {
	if (!packet_debug_enabled || payload == NULL) return;

	const uint8_t* p = (const uint8_t*)payload;
	printf("[PAYLOAD] (%zu bytes):\n", len);
	for (size_t i = 0; i < len; ++i) {
		printf("%02X ", p[i]);
		if ((i + 1) % 16 == 0) printf("\n");
	}
	if (len % 16 != 0) printf("\n");
}

/**
 * serialize_message - Convert a message header and payload into a flat buffer
 *
 * @header: Pointer to slim_msg_header_t (metadata)
 * @payload: Pointer to payload data (can be NULL if payload_length == 0)
 * @out_buf: Destination buffer to write serialized data into
 * @buf_size: Size of out_buf in bytes
 *
 * Return: Total number of bytes written (header + payload),
 * 	   of -1, if out_buf is too small to hold the message
 */
int serialize_message(const slim_msg_header_t* header, const char* topic,
											const void* data, size_t data_len, uint8_t* out_buf,
											size_t buf_size) {
	size_t header_size = sizeof(slim_msg_header_t);

	uint8_t topic_len = 0;
	if (topic != NULL) {
		size_t len = strlen(topic);
		if (len > 255) return -1; // topic too big
		topic_len = (uint8_t)len;
	}

	size_t total_payload = 1 + topic_len + data_len;
	
	// check if output buffer is large enough
	if (buf_size < header_size + total_payload) {
		return -1;
	}

	// copy message header into the beginning of the buffer
	memcpy(out_buf, header, header_size);
	// compose payload
	uint8_t* payload_ptr = out_buf + header_size;
	payload_ptr[0] = topic_len;
	
	if (topic_len > 0)
		memcpy(payload_ptr + 1, topic, topic_len);

	if (data_len > 0)
		memcpy(payload_ptr + 1 + topic_len, data, data_len);

	return (int)(header_size + total_payload);
}

/**
 * serialize_control_message - Serialize a MSG_CONTROL packet into a flat byte buffer
 *
 * @header: pointer to the base message header (msg_type will be overwritten as MSG_CONTROL
 * @ctrl_type: control type identifier(CONTROL_HEARTBEAT, CONTROL_DISCONNECT)
 * @data: optional payload data(can be NULL)
 * @data_len: length of the optional data in bytes
 * @buffer: Output buffer to write the serialized packet
 * @buffer_len: Size of the output buffer in bytes
 *
 * Return: Number of bytes written (header + 1 + data), or -1 on error
 */
int serialize_control_message(const slim_msg_header_t* header, control_type_t ctrl_type,
															const void* data, size_t data_len,
															uint8_t* buffer, size_t buffer_len) {
 	if (!header || !buffer || buffer_len < sizeof(slim_msg_header_t) + 1 + data_len) return -1;

	slim_msg_header_t hdr = *header;
	hdr.msg_type = MSG_CONTROL;
	hdr.payload_length = 1 + data_len;

	memcpy(buffer, &hdr, sizeof(slim_msg_header_t));
	buffer[sizeof(slim_msg_header_t)] = (uint8_t)ctrl_type;

	if (data_len > 0) {
		memcpy(buffer + sizeof(slim_msg_header_t) + 1, data, data_len);
	}

	return sizeof(slim_msg_header_t) + 1 + data_len;
}

/**
 * deserialize_control_message - parse a float buffer into a MSG_CONTROL header and payload
 *
 * @in_buf: raw byte buffer received from socket
 * @buf_len: size of the input buffer
 * @out_header: output pointer to store the parsed header
 * @out_type: output pointer to store optional control data
 * @out_data: output buffer to store optional control data
 * @max_data_len: maximum size of the output data buffer
 *
 * Return: 0 on success, -1 for invalid arguents, -2 for buffer too small, -3 for payload overflow
 */
int deserialize_control_message(const uint8_t* in_buf, size_t buf_len,
																slim_msg_header_t* out_header,
																control_type_t* out_type, void* out_data,
																size_t max_data_len) {
	if (!in_buf || !out_header || !out_type) return -1;

	size_t header_size = sizeof(slim_msg_header_t);
	if (buf_len < header_size + 1) return -2;

	memcpy(out_header, in_buf, header_size);

	const uint8_t* payload_ptr = in_buf + header_size;
	*out_type = (control_type_t)payload_ptr[0];

	size_t data_len = out_header->payload_length - 1;
	if (data_len > 0) {
		if (data_len > max_data_len) return -3;
		memcpy(out_data, payload_ptr + 1, data_len);
	}

	return 0;
}

/**
 * deserialize_message - Parse a flat buffer into header and payload
 *
 * @in_buf: Pointer to raw buffer received (contains header + payload)
 * @buf_len: Length of the buffer in bytes
 * @out_header: Output pointer to store parsed message header
 * @output_payload: Output buffer to store parsed payload
 * @max_payload: Maximum size that out_payload can hold
 *
 * Return: 0 on success,
 * 	   -1 if buffer is too small to contain a header,
 * 	   -2 if payload size is too large of incomplete
 */
int deserialize_message(const uint8_t* in_buf, size_t buf_len,
												slim_msg_header_t* out_header, char* out_topic,
												size_t topic_buf_size, void* out_data,
												size_t max_data_len) {
	size_t header_size = sizeof(slim_msg_header_t);
	
	// verify buffer is large enough to contain at least a header
	if (buf_len < header_size) {
		return -1;
	}

	// copy header portion from buffer
	memcpy(out_header, in_buf, header_size);

	if (out_header->msg_type == MSG_ACK || out_header->msg_type == MSG_CONTROL) {
		size_t data_len = out_header->payload_length;
		if (data_len > 0 && out_data && max_data_len >= data_len) {
			memcpy(out_data, in_buf + header_size, data_len);
		}
		if (out_topic) out_topic[0] = '\0';
		return 0;
	}

	const uint8_t* payload_ptr = in_buf + header_size;
	uint8_t topic_len = payload_ptr[0];

	if ((size_t)(topic_len + 1) > out_header->payload_length) return -2;

	if (out_topic && topic_len > 0) {
		if (topic_len >= topic_buf_size) return -3;
		memcpy(out_topic, payload_ptr + 1, topic_len);
		out_topic[topic_len] = '\0';
	} else if (out_topic && topic_len == 0) {
		out_topic[0] = '\0';
	}
	size_t data_len = out_header->payload_length - (1 + topic_len);
	if (out_data && data_len > 0) {
		if (data_len > max_data_len) return -4;
		memcpy(out_data, payload_ptr + 1 + topic_len, data_len);
	}

	return 0;
}
