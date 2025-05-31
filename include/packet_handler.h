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
															uint8_t* buffer, size_t buffer_len);

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
																size_t max_data_len);
