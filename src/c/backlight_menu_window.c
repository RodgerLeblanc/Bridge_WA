#ifndef PBL_PLATFORM_APLITE
#include <pebble.h>
#include "backlight_menu_window.h"
#include "commons.h"

static Window *s_radio_button_main_window;
static MenuLayer *s_radio_button_menu_layer;

static uint8_t s_radio_button_current_selection = 0;

static char s_radio_button_menu_items[BACKLIGHT_RADIO_BUTTON_WINDOW_NUM_ROWS][16] = { "OFF/Auto", "Always ON" };

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return BACKLIGHT_RADIO_BUTTON_WINDOW_NUM_ROWS;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
    menu_cell_basic_draw(ctx, cell_layer, s_radio_button_menu_items[(int)cell_index->row], NULL, NULL);

    GRect bounds = layer_get_bounds(cell_layer);
    GPoint p = GPoint(bounds.size.w - (PBL_IF_ROUND_ELSE(6, 3) * BACKLIGHT_RADIO_BUTTON_WINDOW_RADIO_RADIUS), (bounds.size.h / 2));

    // Selected?
    if(menu_cell_layer_is_highlighted(cell_layer)) {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_context_set_fill_color(ctx, GColorWhite);
    } else {
      graphics_context_set_fill_color(ctx, GColorBlack);
    }

    // Draw radio filled/empty
    graphics_draw_circle(ctx, p, BACKLIGHT_RADIO_BUTTON_WINDOW_RADIO_RADIUS);
    if(cell_index->row == s_radio_button_current_selection) {
      // This is the selection
      graphics_fill_circle(ctx, p, BACKLIGHT_RADIO_BUTTON_WINDOW_RADIO_RADIUS - 3);
    }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return PBL_IF_ROUND_ELSE(
    menu_layer_is_index_selected(menu_layer, cell_index) ? 
      MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
    44);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  s_radio_button_current_selection = cell_index->row;
  persist_write_int(CONFIG_KEY_BACKLIGHT, cell_index->row);
  menu_layer_reload_data(menu_layer);
	
  s_light_on = (persist_read_int(CONFIG_KEY_BACKLIGHT) == (int)BACKLIGHT_ENABLED);
  light_enable(s_light_on);
  //light_enable(false);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "s_light_on: %s", s_light_on ? "ON" : "OFF");
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_radio_button_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_radio_button_menu_layer, window);
  menu_layer_set_highlight_colors(s_radio_button_menu_layer, main_color, text_color);
  menu_layer_set_callbacks(s_radio_button_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_callback,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_radio_button_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_radio_button_menu_layer);

  window_destroy(window);
  s_radio_button_main_window = NULL;
}

void backlight_menu_window_push() {
  if(!s_radio_button_main_window) {
	if (!persist_exists(CONFIG_KEY_BACKLIGHT)) 
		persist_write_int(CONFIG_KEY_BACKLIGHT, BACKLIGHT_ENABLED);
	s_radio_button_current_selection = persist_read_int(CONFIG_KEY_BACKLIGHT);  
    s_radio_button_main_window = window_create();
    window_set_window_handlers(s_radio_button_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
  }
  window_stack_push(s_radio_button_main_window, true);
}
#endif