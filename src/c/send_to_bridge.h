#pragma once

#include <pebble.h>
#include "commons.h"

void bridge_ask_to_send_data(enum UpdateRequestKey what_to_update);
void send_cstring_to_bridge(const uint32_t key, const char *const value);
void send_uint8_to_bridge(const uint32_t key, const uint8_t value);
void send_email_body_request_to_bridge(const char *const accountid, const char *const messageid, const bool is_email);
void send_outbox_begin();
void send_dict_write_cstring(const uint32_t key, const char *const value);
void send_dict_write_uint8(const uint32_t key, const uint8_t value);
void send_outbox_send();
