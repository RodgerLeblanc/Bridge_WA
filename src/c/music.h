#pragma once

#include "pebble.h"

#define REPEAT_INTERVAL_MS 50

#ifdef PBL_BW
  #define SIZE 50
#else
  #define SIZE 100
#endif

char s_music_album_text[SIZE];// = "Album";
char s_music_artist_text[SIZE];// = "Artist";
char s_music_song_text[SIZE];// = "Song";
bool s_music_is_playing;

void music_update_all_text_layer();
void music_init();
void music_deinit();
bool music_window_stack_contains_music_window();
/*
static void music_previous_click_handler(ClickRecognizerRef recognizer, void *context);
static void music_play_click_handler(ClickRecognizerRef recognizer, void *context);
static void music_next_click_handler(ClickRecognizerRef recognizer, void *context);
static void music_previous_double_click_handler(ClickRecognizerRef recognizer, void *context);
static void music_play_double_click_handler(ClickRecognizerRef recognizer, void *context);
static void music_next_double_click_handler(ClickRecognizerRef recognizer, void *context);
static void music_click_config_provider(void *context);
static void music_main_window_load(Window *window);
static void music_main_window_unload(Window *window);
*/