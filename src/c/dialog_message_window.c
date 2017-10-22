#include "dialog_message_window.h"
#include "commons.h"

typedef struct {
	uint8_t icon;
	char *message;
} DialogValues;

static Window *s_dialog_window;
static TextLayer *s_dialog_label_layer;
static Layer *s_dialog_background_layer;

#ifndef PBL_PLATFORM_APLITE
  static Animation *s_dialog_appear_anim = NULL;
  static BitmapLayer *s_dialog_icon_layer;
  static GBitmap *s_dialog_icon_bitmap;
#endif

static AppTimer *s_dialog_killer_timer = NULL;

#ifndef PBL_PLATFORM_APLITE
static void dialog_anim_stopped_handler(Animation *animation, bool finished, void *context) {
  s_dialog_appear_anim = NULL;
}
#endif

static void dialog_background_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(main_color, GColorWhite));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, 0);
}

static void dialog_window_load(Window *window) {
	DialogValues *dv = (DialogValues *)window_get_user_data(window);
	
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_dialog_background_layer = layer_create(bounds);
  layer_set_update_proc(s_dialog_background_layer, dialog_background_update_proc);
  layer_add_child(window_layer, s_dialog_background_layer);

  GRect bitmap_bounds = GRect(0, 0, 0, 0);
  #ifndef PBL_PLATFORM_APLITE
  s_dialog_icon_bitmap = gbitmap_create_with_resource(dv->icon);
  bitmap_bounds = gbitmap_get_bounds(s_dialog_icon_bitmap);

  s_dialog_icon_layer = bitmap_layer_create(GRect((bounds.size.w/2)-(bitmap_bounds.size.w/2), PBL_IF_BW_ELSE(20, bounds.size.h+10), bitmap_bounds.size.w, bitmap_bounds.size.h));
  bitmap_layer_set_bitmap(s_dialog_icon_layer, s_dialog_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_dialog_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_dialog_icon_layer));
  #endif
  
  s_dialog_label_layer = text_layer_create(GRect(10, PBL_IF_BW_ELSE(20, bounds.size.h+10) + bitmap_bounds.size.h + 5, bounds.size.w-20, bounds.size.h-(10 + bitmap_bounds.size.h + 10)));
  text_layer_set_text(s_dialog_label_layer, dv->message);
	//text_layer_set_text_color(s_dialog_label_layer, text_color);
  text_layer_set_background_color(s_dialog_label_layer, GColorClear);
  text_layer_set_text_alignment(s_dialog_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_dialog_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_dialog_label_layer));
	
	//text_layer_enable_screen_text_flow_and_paging(s_dialog_label_layer, 2);
}

static void dialog_window_unload(Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Dialog1");
  DialogValues *dv = (DialogValues*)window_get_user_data(window);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "2");
//  free(dv->message);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "3");
  free(dv);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "4");
  
	if (s_dialog_killer_timer)
		app_timer_cancel(s_dialog_killer_timer);
	
  layer_destroy(s_dialog_background_layer);

  text_layer_destroy(s_dialog_label_layer);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "5");
	
  #ifndef PBL_PLATFORM_APLITE
  bitmap_layer_destroy(s_dialog_icon_layer);
  gbitmap_destroy(s_dialog_icon_bitmap);
  #endif
  
  window_destroy(window);
  s_dialog_window = NULL;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "6");
}

#ifndef PBL_PLATFORM_APLITE
static void dialog_window_appear(Window *window) {
  if(s_dialog_appear_anim) {
     // In progress, cancel
    animation_unschedule(s_dialog_appear_anim);
  }

  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  GRect bitmap_bounds = gbitmap_get_bounds(s_dialog_icon_bitmap);

  Layer *label_layer = text_layer_get_layer(s_dialog_label_layer);
  Layer *icon_layer = bitmap_layer_get_layer(s_dialog_icon_layer);

	GRect icon_layer_frame = layer_get_frame(icon_layer);
	GRect label_layer_frame = layer_get_frame(label_layer);
	GSize label_layer_size = text_layer_get_content_size(s_dialog_label_layer);
	int icon_y_origin_for_center = (bounds.size.h/2)-((icon_layer_frame.size.h + 5 + label_layer_size.h)/2);
	
  GRect start = layer_get_frame(s_dialog_background_layer);
  GRect finish = bounds;
  Animation *background_anim = (Animation*)property_animation_create_layer_frame(s_dialog_background_layer, &start, &finish);

  start = icon_layer_frame;
  finish = GRect(start.origin.x, icon_y_origin_for_center, bitmap_bounds.size.w, bitmap_bounds.size.h);
  Animation *icon_anim = (Animation*)property_animation_create_layer_frame(icon_layer, &start, &finish);

  start = label_layer_frame;
  finish = GRect(start.origin.x, icon_y_origin_for_center + bitmap_bounds.size.h + 5, start.size.w, start.size.h);
  Animation *label_anim = (Animation*)property_animation_create_layer_frame(label_layer, &start, &finish);

  s_dialog_appear_anim = animation_spawn_create(background_anim, icon_anim, label_anim, NULL);
  animation_set_handlers(s_dialog_appear_anim, (AnimationHandlers) {
    .stopped = dialog_anim_stopped_handler
  }, NULL);
  animation_set_delay(s_dialog_appear_anim, 700);
  animation_schedule(s_dialog_appear_anim);
}
#endif

void dialog_killer_callback(void *data) {
	window_stack_remove(s_dialog_window, true);
}

void dialog_message_show(uint8_t icon, char *message, bool autokill) {
  if(!s_dialog_window) {
	DialogValues *dv = malloc(sizeof(DialogValues));
	dv->icon = icon;
	dv->message = message;
    s_dialog_window = window_create();
	window_set_user_data(s_dialog_window, dv);
    window_set_window_handlers(s_dialog_window, (WindowHandlers) {
        .load = dialog_window_load,
        .unload = dialog_window_unload,
      #ifndef PBL_PLATFORM_APLITE
        .appear = dialog_window_appear
      #endif
    });
  }
  window_stack_push(s_dialog_window, true);
	
  if (autokill)
	s_dialog_killer_timer = app_timer_register(3000, dialog_killer_callback, NULL);
}