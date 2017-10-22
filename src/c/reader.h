#pragma once

#include <pebble.h>

#ifdef PBL_PLATFORM_APLITE
  #define READER_BASE_SIZE 30
#else
  #define READER_BASE_SIZE 100
#endif

void reader_window_create(char *sender, char *subject, char *body, char *accountid, char *messageid, bool is_email, bool is_read);
