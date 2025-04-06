#pragma once

#include <stddef.h>
#include <stdint.h>
#include "nano_msg.h"

int serialize_message(const nano_msg_header_t* header, const void* payload, uint8_t* out_buf, size_t buf_size);

int deserialize_message(const uint8_t* in_buf, size_t buf_len, nano_msg_header_t* out_header, void* out_payload, size_t max_payload);

