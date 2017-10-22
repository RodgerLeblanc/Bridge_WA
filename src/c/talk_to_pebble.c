#ifndef PBL_PLATFORM_APLITE

#include "send_to_bridge.h"
#include "commons.h"
#include "talk_to_pebble.h"

static Window *s_talk_main_window;
static TextLayer *talk_message_layer;

static StatusBarLayer *s_talk_status_bar;

static DictationSession *s_talk_dictation_session;
static char s_talk_display_message[512];

static void talk_deinit();
/*
static void talk_align_vertically(TextLayer* text_layer);
static void talk_dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context);
static void talk_select_click_handler(ClickRecognizerRef recognizer, void *context);
static void talk_click_config_provider(void *context);
static void talk_window_load(Window *window);		
static void talk_window_unload(Window *window);
*/

GRect bounds;
void (*callback_function)(char *transcription, void *context_passed_back);

static void talk_align_vertically(TextLayer* text_layer) {
	GSize layer_size = text_layer_get_content_size(text_layer);
	GRect bounds = layer_get_bounds(text_layer_get_layer(text_layer));
	layer_set_frame(text_layer_get_layer(text_layer), GRect(0, (bounds.size.h/2)-(layer_size.h/2), bounds.size.w, bounds.size.h));
}

static void talk_dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
	  callback_function(transcription, context);
    snprintf(s_talk_display_message, sizeof(s_talk_display_message), "Message sent!\n\n\"%s\"", transcription);
    text_layer_set_text(talk_message_layer, s_talk_display_message);
	talk_align_vertically(talk_message_layer);
	//send_cstring_to_bridge(s_command_key, transcription);
	window_stack_remove(s_talk_main_window, true);
  } else {
    static char error_message[128];
    snprintf(error_message, sizeof(error_message), "Error code: %d\n\nPress select to retry.", (int)status);
    text_layer_set_text(talk_message_layer, error_message);
	talk_align_vertically(talk_message_layer);
  }
}

static void talk_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  dictation_session_start(s_talk_dictation_session);
}

static void talk_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, talk_select_click_handler);
}

static void talk_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
	
  s_talk_status_bar = status_bar_layer_create();
  #ifdef PBL_COLOR
	  status_bar_layer_set_colors(s_talk_status_bar, main_color, GColorBlack);
  #endif
  layer_add_child(window_layer, status_bar_layer_get_layer(s_talk_status_bar));
	
  bounds = layer_get_bounds(window_layer);

  talk_message_layer = text_layer_create(GRect(bounds.origin.x, STATUS_BAR_LAYER_HEIGHT + bounds.origin.y, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
	text_layer_set_text(talk_message_layer, s_talk_display_message);
 // text_layer_set_text(talk_message_layer, "Take short Remember notes using voice technology.\nPress SELECT to start.");
  text_layer_set_text_alignment(talk_message_layer, GTextAlignmentCenter);
  text_layer_set_font(talk_message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(talk_message_layer));
  talk_align_vertically(talk_message_layer);
	
  #ifdef PBL_ROUND
	text_layer_enable_screen_text_flow_and_paging(talk_message_layer, 2);
  #endif
	
	//if (strcmp(s_talk_display_message, "") == 0)
	//	dictation_session_start(s_talk_dictation_session);
}
		
static void talk_window_unload(Window *window) {
  status_bar_layer_destroy(s_talk_status_bar);
  text_layer_destroy(talk_message_layer);
  talk_deinit();
}

void talk_init(char *message_to_display, void (*f)(char *transcription, void *context_passed_back), void *context) {
  bool show_init_message = (strcmp(message_to_display, "") != 0);
  callback_function = f;
  strncpy(s_talk_display_message, message_to_display, sizeof(s_talk_display_message));
  s_talk_dictation_session = dictation_session_create(sizeof(s_talk_display_message), talk_dictation_session_callback, context);
  s_talk_main_window = window_create();
  window_set_click_config_provider(s_talk_main_window, talk_click_config_provider);
  window_set_window_handlers(s_talk_main_window, (WindowHandlers) {
    .load = talk_window_load,
    .unload = talk_window_unload,
  });
  window_stack_push(s_talk_main_window, show_init_message);

  if (!show_init_message)
	dictation_session_start(s_talk_dictation_session);
}

static void talk_deinit() {
  dictation_session_destroy(s_talk_dictation_session);

  if (s_talk_main_window)
  	window_destroy(s_talk_main_window);
}

#endif