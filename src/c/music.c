#include "music.h"
#include "commons.h"
#include "send_to_bridge.h"

static Window *s_music_main_window;

static ActionBarLayer *s_music_action_bar;
static TextLayer *s_album_layer, *s_artist_layer, *s_song_layer;
static GBitmap *s_icon_previous, *s_icon_play, *s_icon_pause, *s_icon_next;

static StatusBarLayer *music_status_bar;

static uint8_t update_retry = 0;

void music_update_all_text_layer() {
	if (s_album_layer)
		text_layer_set_text(s_album_layer, s_music_album_text);
	if (s_artist_layer)
		text_layer_set_text(s_artist_layer, s_music_artist_text);
	if (s_song_layer)
		text_layer_set_text(s_song_layer, s_music_song_text);
	
	action_bar_layer_set_icon(s_music_action_bar, BUTTON_ID_SELECT, s_music_is_playing ? s_icon_pause : s_icon_play);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "s_music_is_playing: %s", s_music_is_playing ? "true" : "false");
	
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) {
		update_retry++;
		if (update_retry <= 3) {
			psleep(200);
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
		}
		else {
			update_retry = 0;
		}
	}
}

static void music_previous_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_uint8_to_bridge(MUSIC_COMMAND_KEY, MUSIC_PREVIOUS);
	update_retry = 0;
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) 
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

static void music_play_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_uint8_to_bridge(MUSIC_COMMAND_KEY, MUSIC_PLAY);
	update_retry = 0;
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) 
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

static void music_next_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_uint8_to_bridge(MUSIC_COMMAND_KEY, MUSIC_NEXT);
	update_retry = 0;
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) 
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

static void music_previous_double_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_uint8_to_bridge(MUSIC_COMMAND_KEY, MUSIC_VOLUME_UP);
	update_retry = 0;
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) 
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

static void music_play_double_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_uint8_to_bridge(MUSIC_COMMAND_KEY, MUSIC_STOP);
	update_retry = 0;
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) 
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

static void music_next_double_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_uint8_to_bridge(MUSIC_COMMAND_KEY, MUSIC_VOLUME_DOWN);
	update_retry = 0;
	if ((strcmp(s_music_album_text, "") == 0) && (strcmp(s_music_artist_text, "") == 0) && (strcmp(s_music_song_text, "") == 0)) 
			bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

static void music_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, music_previous_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, music_play_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, music_next_click_handler);

  window_multi_click_subscribe(BUTTON_ID_UP, 2, 0, 0, false, music_previous_double_click_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 0, 0, false, music_play_double_click_handler);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 0, 0, false, music_next_double_click_handler);
}

static void music_main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  music_status_bar = status_bar_layer_create();
  #ifdef PBL_COLOR
	  status_bar_layer_set_colors(music_status_bar, main_color, GColorBlack);
  #endif
  layer_add_child(window_layer, status_bar_layer_get_layer(music_status_bar));
	
  s_music_action_bar = action_bar_layer_create();
  action_bar_layer_set_background_color(s_music_action_bar, main_color);
  action_bar_layer_add_to_window(s_music_action_bar, window);
  action_bar_layer_set_click_config_provider(s_music_action_bar, music_click_config_provider);

  action_bar_layer_set_icon(s_music_action_bar, BUTTON_ID_UP, s_icon_previous);
  action_bar_layer_set_icon(s_music_action_bar, BUTTON_ID_SELECT, s_music_is_playing ? s_icon_pause : s_icon_play);
  action_bar_layer_set_icon(s_music_action_bar, BUTTON_ID_DOWN, s_icon_next);

  uint8_t width = layer_get_frame(window_layer).size.w - ACTION_BAR_WIDTH - 3;

  s_album_layer = text_layer_create(GRect(4, STATUS_BAR_LAYER_HEIGHT, width, 40));
  text_layer_set_font(s_album_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_album_layer, GColorClear);
  text_layer_set_text_alignment(s_album_layer, GTextAlignmentCenter);
  text_layer_set_text(s_album_layer, s_music_album_text);
  layer_add_child(window_layer, text_layer_get_layer(s_album_layer));

  s_artist_layer = text_layer_create(GRect(4, STATUS_BAR_LAYER_HEIGHT+40, width, 60));
  text_layer_set_font(s_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_artist_layer, GColorClear);
  text_layer_set_text_alignment(s_artist_layer, GTextAlignmentCenter);
  text_layer_set_text(s_artist_layer, s_music_artist_text);
  layer_add_child(window_layer, text_layer_get_layer(s_artist_layer));

  s_song_layer = text_layer_create(GRect(4, STATUS_BAR_LAYER_HEIGHT + 40 + 28 + 28, width, 60));
  text_layer_set_font(s_song_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(s_song_layer, GColorClear);
  text_layer_set_text_alignment(s_song_layer, GTextAlignmentCenter);
  text_layer_set_text(s_song_layer, s_music_song_text);
  layer_add_child(window_layer, text_layer_get_layer(s_song_layer));
	
  #ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_album_layer, 2);
  text_layer_enable_screen_text_flow_and_paging(s_artist_layer, 2);
  text_layer_enable_screen_text_flow_and_paging(s_song_layer, 2);
  #endif
}

static void music_main_window_unload(Window *window) {
  text_layer_destroy(s_album_layer);
  text_layer_destroy(s_artist_layer);
  text_layer_destroy(s_song_layer);

  action_bar_layer_destroy(s_music_action_bar);
  status_bar_layer_destroy(music_status_bar);
	
  music_deinit();
}

void music_init() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "music_init()");
  s_icon_previous = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ACTION_PREVIOUS);
  s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ACTION_PLAY);
  s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ACTION_PAUSE);
  s_icon_next = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ACTION_NEXT);

  //strncpy(s_music_album_text, "Album", 100);
  //strncpy(s_music_artist_text, "Artist", 100);
  //strncpy(s_music_song_text, "Song", 100);

  s_music_main_window = window_create();
  window_set_window_handlers(s_music_main_window, (WindowHandlers) {
    .load = music_main_window_load,
    .unload = music_main_window_unload,
  });

  window_stack_push(s_music_main_window, true);
  music_update_all_text_layer();
  bridge_ask_to_send_data(UPDATE_MUSIC_METADATA);
}

void music_deinit() {
  gbitmap_destroy(s_icon_previous);
  gbitmap_destroy(s_icon_play);
  gbitmap_destroy(s_icon_pause);
  gbitmap_destroy(s_icon_next);
	
  if (s_music_main_window)
	window_destroy(s_music_main_window);
}

bool music_window_stack_contains_music_window() {
  return window_stack_contains_window(s_music_main_window);
}