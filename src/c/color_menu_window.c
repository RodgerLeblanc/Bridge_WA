/**
 * Example implementation of the radio button list UI pattern.
 */

#ifdef PBL_COLOR

#include "color_menu_window.h"

#include "commons.h"
#include "dialog_message_window.h"
#include "vibration_control.h"

static Window *s_main_window;
static MenuLayer *s_menu_layer;

static uint8_t s_current_selection = 0;

static char s_menu_items[COLOR_RADIO_BUTTON_WINDOW_NUM_ROWS][25] = { "Black", "Oxford Blue", "Duke Blue", "Blue", "Dark Green", "Midnight Green", "Cobalt Blue", "Blue Moon",
															  "Islamic Green", "Jaeger Green", "Tiffany Blue", "Vivid Cerulean", "Green", "Malachite",
															  "Medium Spring Green", "Cyan", "Bulgarian Rose", "Imperial Purple", "Indigo", "Electric Ultramarine",
															  "Army Green", "Dark Gray", "Liberty", "Very Light Blue", "Kelly Green", "May Green", "Cadet Blue",
															  "Picton Blue", "Bright Green", "Screamin Green", "Medium Aquamarine", "Electric Blue",
															  "Dark Candy Apple Red", "Jazzberry Jam", "Purple", "Vivid Violet", "Windsor Tan", "Rose Vale",
															  "Purpureus", "Lavender Indigo", "Limerick", "Brass", "Light Gray", "Baby Blue Eyes", "Spring Bud",
															  "Inchworm", "Mint Green", "Celeste", "Red", "Folly", "Fashion Magenta", "Magenta", "Orange", 
															  "Sunset Orange", "Brilliant Rose", "Shocking Pink", "Chrome Yellow", "Rajah", "Melon", 
															  "Rich Brilliant Lavender", "Yellow", "Icterine", "Pastel Yellow", "White"
															  };

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return COLOR_RADIO_BUTTON_WINDOW_NUM_ROWS;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
    menu_cell_basic_draw(ctx, cell_layer, s_menu_items[(int)cell_index->row], NULL, NULL);

    GRect bounds = layer_get_bounds(cell_layer);
    GPoint p;
	#ifdef PBL_ROUND
		p = GPoint((bounds.size.w / 2) - COLOR_RADIO_BUTTON_WINDOW_RADIO_RADIUS, (bounds.size.h / 2) + 24);
	#else
		p = GPoint(bounds.size.w - (3 * COLOR_RADIO_BUTTON_WINDOW_RADIO_RADIUS), (bounds.size.h / 2));
	#endif

    // Selected?
    if(menu_cell_layer_is_highlighted(cell_layer)) {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_context_set_fill_color(ctx, GColorWhite);
    } else {
      graphics_context_set_fill_color(ctx, GColorBlack);
    }

    // Draw radio filled/empty
    graphics_draw_circle(ctx, p, COLOR_RADIO_BUTTON_WINDOW_RADIO_RADIUS);
    if(cell_index->row == s_current_selection) {
      // This is the selection
      graphics_fill_circle(ctx, p, COLOR_RADIO_BUTTON_WINDOW_RADIO_RADIUS - 3);
    }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return PBL_IF_ROUND_ELSE(MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT, 44);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  #ifdef PBL_COLOR
    s_current_selection = cell_index->row;
	  main_color.argb = 192 + cell_index->row;
	  persist_write_int(CONFIG_KEY_MAIN_COLOR, main_color.argb);
	
	  menu_layer_set_highlight_colors(menu_layer, main_color, text_color);
    menu_layer_reload_data(menu_layer);
  #endif
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
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);

  window_destroy(window);
  s_main_window = NULL;
}

void color_menu_window_push() {
  if(!s_main_window) {
	if (!persist_exists(CONFIG_KEY_MAIN_COLOR)) {
		persist_write_int(CONFIG_KEY_MAIN_COLOR, GColorFashionMagenta.argb);
		main_color = PBL_IF_COLOR_ELSE(GColorFashionMagenta, GColorBlack);
	}
	s_current_selection = persist_read_int(CONFIG_KEY_MAIN_COLOR) - 192;
    
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
  }
  window_stack_push(s_main_window, true);
}

#endif