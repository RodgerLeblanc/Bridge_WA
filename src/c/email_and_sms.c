#include "email_and_sms.h"
#include "commons.h"
#include "reader.h"
#include "send_to_bridge.h"

static Window *s_message_main_window;
static SimpleMenuLayer *s_message_simple_menu_layer;
static SimpleMenuSection s_message_menu_sections[MESSAGE_NUM_MENU_SECTIONS];
static SimpleMenuItem s_message_first_menu_items[MESSAGE_MAX_MENU_ITEMS];
#ifndef PBL_PLATFORM_APLITE
static GBitmap *s_message_menu_icon_image[MESSAGE_MAX_MENU_ITEMS];
#endif

static StatusBarLayer *s_message_status_bar;

static uint8_t message_num_a_items = 0;
static bool s_send_email = true;
static int8_t message_item_clicked = -1;

static void message_deinit();

static void message_menu_select_callback(int index, void *ctx) {	
	message_item_clicked = index;	
	send_email_body_request_to_bridge(message_array_of_accountid[index], message_array_of_messageid[index], message_array_of_is_email[index]);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message_array_of_accountid: %s", message_array_of_accountid[index]);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message_array_of_messageid: %s", message_array_of_messageid[index]);

	message_array_of_is_read[index] = true;
	
	SimpleMenuItem *menu_item = &s_message_first_menu_items[index];
	menu_item->title = "LOADING...";
	menu_item->subtitle = "Please wait";
	#ifndef PBL_PLATFORM_APLITE
	menu_item->icon = s_message_menu_icon_image[index];
	#endif
	layer_mark_dirty(simple_menu_layer_get_layer(s_message_simple_menu_layer));
}

void message_initialize_menu_items() {
	for (uint8_t i = 0; i < MESSAGE_MAX_MENU_ITEMS; i++) {
		strcpy(message_array_of_subject[i], "LOADING...");
		strcpy(message_array_of_sender[i], "");
		strcpy(message_array_of_accountid[i], "");
		strcpy(message_array_of_messageid[i], "");
		message_array_of_is_email[i] = s_send_email;
		message_array_of_is_read[i] = true;
	}
}

void populate_menu_items() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "email.c populate_menu_items");
	for (uint8_t i = 0; i < MESSAGE_MAX_MENU_ITEMS; i++) {
		#ifndef PBL_PLATFORM_APLITE
		if (message_array_of_accountid[i] == 0)
			s_message_menu_icon_image[i] = NULL;
		else
  			s_message_menu_icon_image[i] = gbitmap_create_with_resource(message_array_of_is_email[i] ? (message_array_of_is_read[i] ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_EMAIL_UNREAD) : (message_array_of_is_read[i] ? RESOURCE_ID_IMAGE_PEBBLE_SMS_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_UNREAD));
		#endif
		
      s_message_first_menu_items[i] = (SimpleMenuItem) {
    		.title = (strcmp(message_array_of_subject[i], "")==0) ? "[EMPTY SUBJECT]" : message_array_of_subject[i],
			.subtitle = message_array_of_sender[i],
		    #ifndef PBL_PLATFORM_APLITE
			.icon = s_message_menu_icon_image[i],
		    #endif
    		.callback = message_menu_select_callback,
		  };
	}
	
  s_message_menu_sections[0] = (SimpleMenuSection) {
    .num_items = MESSAGE_MAX_MENU_ITEMS,
    .items = s_message_first_menu_items,
  };
  
  Layer *message_window_layer = window_get_root_layer(s_message_main_window);
	
  s_message_status_bar = status_bar_layer_create();
  #ifdef PBL_COLOR
  status_bar_layer_set_colors(s_message_status_bar, main_color, GColorBlack);
  #endif
  layer_add_child(message_window_layer, status_bar_layer_get_layer(s_message_status_bar));
	
  GRect bounds = layer_get_frame(message_window_layer);
  bounds.origin.y = STATUS_BAR_LAYER_HEIGHT;
  s_message_simple_menu_layer = simple_menu_layer_create(bounds, s_message_main_window, s_message_menu_sections, MESSAGE_NUM_MENU_SECTIONS, NULL);
  layer_add_child(message_window_layer, simple_menu_layer_get_layer(s_message_simple_menu_layer));
  menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_message_simple_menu_layer), PBL_IF_BW_ELSE(GColorBlack, main_color), PBL_IF_BW_ELSE(GColorWhite, text_color));
}

void message_update_menu(uint8_t item_number) {
  	if (s_message_simple_menu_layer) {
		#ifndef PBL_PLATFORM_APLITE
		if (s_message_menu_icon_image[item_number]) {
			gbitmap_destroy(s_message_menu_icon_image[item_number]);
			s_message_menu_icon_image[item_number] = NULL;
		}
  		s_message_menu_icon_image[item_number] = gbitmap_create_with_resource(message_array_of_is_email[item_number] ? (message_array_of_is_read[item_number] ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_EMAIL_UNREAD) : (message_array_of_is_read[item_number] ? RESOURCE_ID_IMAGE_PEBBLE_SMS_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_UNREAD));
		#endif
		
		SimpleMenuItem *menu_item = &s_message_first_menu_items[item_number];
    	menu_item->title = (strcmp(message_array_of_subject[item_number], "")==0) ? "[EMPTY SUBJECT]" : message_array_of_subject[item_number];
		menu_item->subtitle = message_array_of_sender[item_number];
    	#ifndef PBL_PLATFORM_APLITE
		menu_item->icon = s_message_menu_icon_image[item_number];
		#endif
				
		layer_mark_dirty(simple_menu_layer_get_layer(s_message_simple_menu_layer));
	}
}

void message_update_all_menu() {
	for (uint8_t i = 0; i < MESSAGE_MAX_MENU_ITEMS; i++) {
		message_update_menu(i);
	}
}

static void message_main_window_disappear(Window *window) {
	message_update_all_menu();
}

static void message_main_window_appear(Window *window) {
	message_update_all_menu();
	bridge_ask_to_send_data(s_send_email ? UPDATE_EMAIL_LIST : UPDATE_SMS_LIST);
}

static void message_main_window_load(Window *window) {
  message_initialize_menu_items();
  populate_menu_items();
}

void message_main_window_unload(Window *window) {
  #ifndef PBL_PLATFORM_APLITE
  for (uint8_t i = 0; i < MESSAGE_MAX_MENU_ITEMS; i++) {
	    if (s_message_menu_icon_image[i]) {
  		  gbitmap_destroy(s_message_menu_icon_image[i]);
		    s_message_menu_icon_image[i] = NULL;
	    }
  }  
  #endif
  simple_menu_layer_destroy(s_message_simple_menu_layer);
  status_bar_layer_destroy(s_message_status_bar);
  message_deinit();
}

bool message_is_window_in_stack() {
	return window_stack_contains_window(s_message_main_window);
}

void message_init(bool send_email) {
  s_send_email = send_email;
  message_num_a_items = 0;
  s_message_main_window = window_create();
  window_set_window_handlers(s_message_main_window, (WindowHandlers) {
    .load = message_main_window_load,
    .unload = message_main_window_unload,
	.disappear = message_main_window_disappear,
    .appear = message_main_window_appear	
  });
  window_stack_push(s_message_main_window, true);
  bridge_ask_to_send_data(send_email ? UPDATE_EMAIL_LIST : UPDATE_SMS_LIST);
}

static void message_deinit() {
  if (s_message_main_window)
  	  window_destroy(s_message_main_window);
  message_initialize_menu_items();
}