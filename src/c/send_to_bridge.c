#include "send_to_bridge.h"

static uint8_t s_send_iterator = 0;
static void *iter_context;

static enum UpdateRequestKey s_send_last_update_request_key = UNSPECIFIED;

void bridge_ask_to_send_data(enum UpdateRequestKey what_to_update) {
  s_send_last_update_request_key = what_to_update;
  
  if (what_to_update == UNSPECIFIED) {
    return;
  }

  // This is to get initial values sent to Bridge WA
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  uint8_t dumb_variable = (uint8_t)what_to_update;
  dict_write_int(iter, REFRESH_NEEDED_KEY, &dumb_variable, sizeof(dumb_variable), true);

  time_t now = time(NULL);
  int transactionId = ((int)now * 10) + s_send_iterator; 
  s_send_iterator++;
  if (s_send_iterator == 254)
    s_send_iterator = 0;
  dict_write_int(iter, TRANSACTION_ID_KEY, &transactionId, sizeof(transactionId), true);
	
  app_message_outbox_send();
}

void send_cstring_to_bridge(const uint32_t key, const char *const value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, key, value);
	
  time_t now = time(NULL);
  int transactionId = ((int)now * 10) + s_send_iterator; 
  s_send_iterator++;
  if (s_send_iterator == 254)
    s_send_iterator = 0;
  dict_write_int(iter, TRANSACTION_ID_KEY, &transactionId, sizeof(transactionId), true);
	
  app_message_outbox_send();
}

void send_uint8_to_bridge(const uint32_t key, const uint8_t value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  int toSend = (int)value;
  dict_write_int(iter, key, &toSend, sizeof(toSend), true);

  time_t now = time(NULL);
  int transactionId = ((int)now * 10) + s_send_iterator; 
  s_send_iterator++;
  if (s_send_iterator == 254)
    s_send_iterator = 0;
  dict_write_int(iter, TRANSACTION_ID_KEY, &transactionId, sizeof(transactionId), true);

  app_message_outbox_send();
}

void send_email_body_request_to_bridge(const char *const accountid, const char *const messageid, const bool is_email) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_ACCOUNTID_KEY, accountid);
  dict_write_cstring(iter, MESSAGE_MESSAGEID_KEY, messageid);
  dict_write_uint8(iter, MESSAGE_IS_EMAIL_KEY, is_email ? 1 : 0);

  time_t now = time(NULL);
  int transactionId = ((int)now * 10) + s_send_iterator; 
  s_send_iterator++;
  if (s_send_iterator == 254)
    s_send_iterator = 0;
  dict_write_int(iter, TRANSACTION_ID_KEY, &transactionId, sizeof(transactionId), true);

  app_message_outbox_send();
}

void send_outbox_begin() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  iter_context = (void*)iter;
}

void send_dict_write_cstring(const uint32_t key, const char *const value) {
  DictionaryIterator *iter = (DictionaryIterator*)iter_context;
  dict_write_cstring(iter, key, value);
}

void send_dict_write_uint8(const uint32_t key, const uint8_t value) {
  DictionaryIterator *iter = (DictionaryIterator*)iter_context;
  dict_write_uint8(iter, key, value);
}

void send_outbox_send() {
  DictionaryIterator *iter = (DictionaryIterator*)iter_context;
  time_t now = time(NULL);
  int transactionId = ((int)now * 10) + s_send_iterator; 
  s_send_iterator++;
  if (s_send_iterator == 254)
    s_send_iterator = 0;
  dict_write_int(iter, TRANSACTION_ID_KEY, &transactionId, sizeof(transactionId), true);

	/*
	Tuple *tuple = dict_read_first(iter);
	while (tuple) {
		if (tuple->type == TUPLE_UINT) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "%d : %d", (int)tuple->key, (int)tuple->value);
		}
		tuple = dict_read_next(iter);
	}
	*/
	
  app_message_outbox_send();
}