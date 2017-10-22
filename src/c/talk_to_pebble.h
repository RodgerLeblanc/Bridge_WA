#ifndef PBL_PLATFORM_APLITE

#pragma once

#include <pebble.h>

void talk_init(char *message_to_display, void (*f)(char *transcription, void *context_passed_back), void *context);

#endif