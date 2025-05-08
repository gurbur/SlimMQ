#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "slim_msg.h"

/**
 * set_packet_debug - Enable/disable hex and payload debug printing
 */
void set_packet_debug(bool enable);

/**
 * dump_header - Print header fields in human-readable format
 */
void dump_header(const slim_msg_header_t* hdr);

/**
 * dump_payload - Print payload in hex format
 */
void dump_payload(const void* payload, size_t len);

/**
 * dump_hex - Print raw buffer in hexadecimal format
 */
void dump_hex(const uint8_t* data, size_t len);

/**
 * serialize_message - Write header + topic + data into a flat buffer
 *
 * Payload format: [topic_len (1 byte)][topic string][message data]
 *
 * @header: pointer to message header (without topic/data)
 * @topic: topic string (null-terminated)
 * @data: message body (can be binary)
 * @data_len: length of the message body
 * @out_buf: output buffer
 * @buf_size: total size of the output buffer
 *
 * Return: number of bytes written, or -1 on failure
 */
int serialize_message(const slim_msg_header_t* header,
                      const char* topic,
                      const void* data, size_t data_len,
                      uint8_t* out_buf, size_t buf_size);

/**
 * deserialize_message - Parse a flat buffer into header, topic, and data
 *
 * Payload format: [topic_len][topic string][data]
 *
 * @in_buf: input buffer containing header + payload
 * @in_len: total length of input buffer
 * @out_header: pointer to store parsed header
 * @out_topic: buffer to write parsed topic string
 * @topic_buf_size: size of topic output buffer
 * @out_data: buffer to write data content
 * @max_data_len: size of data output buffer
 *
 * Return: 0 on success, negative value on error
 */
int deserialize_message(const uint8_t* in_buf, size_t in_len,
                        slim_msg_header_t* out_header,
                        char* out_topic, size_t topic_buf_size,
                        void* out_data, size_t max_data_len);

