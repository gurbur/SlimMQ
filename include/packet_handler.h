#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "nano_msg.h"

void set_packet_debug(bool enable);

void dump_hex(const uint8_t* data, size_t len);

void dump_header(const nano_msg_header_t* hdr);

void dump_payload(const void* payload, size_t len);

int serialize_message(const nano_msg_header_t* header, const void* payload, uint8_t* out_buf, size_t buf_size);

int deserialize_message(const uint8_t* in_buf, size_t buf_len, nano_msg_header_t* out_header, void* out_payload, size_t max_payload);

