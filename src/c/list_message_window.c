/**
 * Example implementation of the list menu UI pattern.
 */

#ifndef PBL_PLATFORM_APLITE

#include "backlight_menu_window.h"
#include "dialog_message_window.h"
#include "list_message_window.h"

#include "commons.h"
#ifdef PBL_COLOR
  #include "color_menu_window.h"
#endif
#include "font_size_menu_window.h"
#include "vibration_menu_window.h"

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static TextLayer *s_list_message_layer;

#ifdef PBL_PLATFORM_APLITE
static char s_menu_items[LIST_MESSAGE_WINDOW_NUM_ROWS][25] = { "Vibration", "Font Size", "Backlight" };
#else
static char s_menu_items[LIST_MESSAGE_WINDOW_NUM_ROWS][25] = { "Vibration", "Font Size", "Color", "Backlight" };
#endif

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return LIST_MESSAGE_WINDOW_NUM_ROWS;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  menu_cell_basic_draw(ctx, cell_layer, s_menu_items[(int)cell_index->row], NULL, NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return PBL_IF_ROUND_ELSE(
    menu_layer_is_index_selected(menu_layer, cell_index) ? 
      MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
    44);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	switch(cell_index->row) {
		case 0: {
			vibration_menu_window_push();
			break;
		}
		case 1: {
			font_size_menu_window_push();
			dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ, "Only applies to Email and SMS reader screen", false);
			break;
		}
		case 2: {
			#ifdef PBL_COLOR
			color_menu_window_push();
			break;
		}
		case 3: {
			#endif
			backlight_menu_window_push();
			break;
		}
		default: break;
	}
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_highlight_colors(s_menu_layer, main_color, text_color);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_callback,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

 // const GEdgeInsets message_insets = {.top = 140};
//  s_list_message_layer = text_layer_create(grect_inset(bounds, message_insets));
 // text_layer_set_text_alignment(s_list_message_layer, GTextAlignmentCenter);
//  text_layer_set_text(s_list_message_layer, LIST_MESSAGE_WINDOW_HINT_TEXT);
 // layer_add_child(window_layer, text_layer_get_layer(s_list_message_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_list_message_layer);

  window_destroy(window);
  s_main_window = NULL;
}

static void main_window_appear(Window *window) {
  if (s_menu_layer)
  	menu_layer_set_highlight_colors(s_menu_layer, main_color, text_color);
}

void list_message_window_push() {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
		.appear = main_window_appear,
    });
  }
  window_stack_push(s_main_window, true);
}

#endif