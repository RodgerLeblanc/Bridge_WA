#pragma once

#include <pebble.h>

enum BacklightValue {
  BACKLIGHT_DISABLED,
  BACKLIGHT_ENABLED
};

bool s_light_on;

#ifndef PBL_PLATFORM_APLITE

#define BACKLIGHT_RADIO_BUTTON_WINDOW_NUM_ROWS     2
#define BACKLIGHT_RADIO_BUTTON_WINDOW_CELL_HEIGHT  44
#define BACKLIGHT_RADIO_BUTTON_WINDOW_RADIO_RADIUS 6

void backlight_menu_window_push();
#endif