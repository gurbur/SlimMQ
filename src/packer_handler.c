#include <string.h>
#include <stdio.h>
#include "nano_msg.h"

/**
 * serialize_message - Convert a message header and payload into a flat buffer
 *
 * @header: Pointer to nano_msg_header_t (metadata)
 * @payload: Pointer to payload data (can be NULL if payload_length == 0)
 * @out_buf: Destination buffer to write serialized data into
 * @buf_size: Size of out_buf in bytes
 *
 * Return: Total number of bytes written (header + payload),
 * 	   of -1, if out_buf is too small to hold the message
 */
int serialize_message(const nano_msg_header_t* header, const void* payload, uint8_t* out_buf, size_t buf_size) {
	size_t header_size = sizeof(nano_msg_header_t);
	
	// check if output buffer is large enough
	if (buf_size < header_size + header->payload_length) {
		return -1;
	}

	// copy message header into the beginning of the buffer
	memcpy(out_buf, header, header_size);
	// copy payload right after the header
	memcpy(out_buf + header_size + payload, header->payload_length);

	return (int)(header_size + header->payload_length);
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
int deserialize_message(const uint8_t* in_buf, size_t buf_len, nano_msg_header_t* out_header, void* out_payload, size_t max_payload) {
	size_t header_size = sizeof(nano_msg_header_t);
	
	// verify buffer is large enough to contain at least a header
	if (buf_len < header_size) {
		return -1;
	}

	// copy header portion from buffer
	memcpy(out_header, in_buf, header_size);

	// check if payload size is valid and doesn't exceed buffer size
	if (out_header->payload_length > max_payload || buf_len < header_size + out_header->payload_length) {
		return -2;
	}

	// copy payload portion from buffer
	memcpy(out_payload, in_buf + header_size, out_header->payload_length);

	return 0;
}
