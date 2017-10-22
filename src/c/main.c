#include "pebble.h"
#include "commons.h"
#include "backlight_menu_window.h"
#include "dialog_message_window.h"
#include "doo_menu_window.h"
#include "email_and_sms.h"
#include "heap_check.h"
#include "music.h"
#include "reader.h"
#include "send_to_bridge.h"
#ifndef PBL_PLATFORM_APLITE
  #include "list_message_window.h"
  #include "talk_to_pebble.h"
#endif
#include "version.h"
#include "vibration_control.h"

#define NUM_MENU_SECTIONS 1
#define MAX_MENU_ITEMS 7

static Window *s_main_window;
static StatusBarLayer *s_status_bar;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[MAX_MENU_ITEMS];
#ifndef PBL_PLATFORM_APLITE
static GBitmap *s_menu_icon_image[MAX_MENU_ITEMS];
#endif
static uint8_t num_a_items = 0;
static bool s_wa_initialized = false;
//static uint8_t s_outbox_retry = 0;
static enum UpdateRequestKey s_update_request_before_initialized = UNSPECIFIED;
static char s_about_message[100];
static int16_t s_inbound_size;
#ifdef PBL_PLATFORM_APLITE
static char s_vibration_text[] = "Vibration: x";
static uint8_t s_vibe;
#endif
  
static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
//  s_outbox_retry = 0;
	

	Tuple *tuple = dict_read_first(iterator);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "** Tuples sent to Bridge **");
	while (tuple) {
		switch (tuple->type) {
			case TUPLE_BYTE_ARRAY:
			case TUPLE_CSTRING: {
				APP_LOG(APP_LOG_LEVEL_DEBUG, "cstring %i : %s", (int)tuple->key, tuple->value->cstring);
				break;
			}
			case TUPLE_UINT: 
			case TUPLE_INT: {
				APP_LOG(APP_LOG_LEVEL_DEBUG, "int %i : %i", (int)tuple->key, tuple->value->int16);
				break;
			}
		}
		tuple = dict_read_next(iterator);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "** END **");
	
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox failed, reason: %d", (int)reason);
//  if (s_outbox_retry++ < 10) {
//    app_message_outbox_send();
//  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Inbox dropped, reason: %d", (int)reason);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  bool music_updated = false;
  bool message_to_show = false;
  char *sender = NULL;
  char *subject = NULL;
  char *body = NULL;
  char *accountid = NULL;
  char *messageid = NULL;
  bool is_email = true;
  bool is_read = true;
  bool is_new_message = false;
  int8_t number_in_array = -1;
	
  Tuple *tuple = dict_read_first(iter);
  while (tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received tuple->key: %d, heap: %d", (int)tuple->key, (int)heap_bytes_free());
  	switch (tuple->key) {
		  case WATCHAPP_READY_SIGNAL: {
			  if (!s_wa_initialized) {
				  s_wa_initialized = true;
				  if (s_update_request_before_initialized != UNSPECIFIED)
					  bridge_ask_to_send_data(s_update_request_before_initialized);
			  }
			  send_outbox_begin();
			  send_dict_write_cstring(WATCHAPP_STARTED, tuple->value->cstring);
			  char inbound[] = "xxxx";
			  snprintf(inbound, sizeof(inbound), "%i", s_inbound_size);
			  send_dict_write_cstring(INBOUND_SIZE, inbound);
			 // send_dict_write_uint8(INBOUND_SIZE, s_inbound_size);
			  char api[] = "xx.xx";
			  snprintf(api, sizeof(api), "%s", BRIDGE_API_VERSION);
			  send_dict_write_cstring(IS_BRIDGE_LISTENING_KEY, api);
			  send_outbox_send();
			  break;
		  }
		case BRIDGE_IS_LISTENING: {
        	if ((s_status_bar) && (window_stack_contains_window(s_main_window))) {
          		APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting colors to status bar: %d %d", main_color.argb, text_color.argb);
          		status_bar_layer_set_colors(s_status_bar, main_color, text_color);
			}
			else {
				Layer *window_layer = window_get_root_layer(s_main_window);
				s_status_bar = status_bar_layer_create();
				status_bar_layer_set_colors(s_status_bar, main_color, text_color);
				layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
			}
			break;
		}
    	case ALBUM_KEY: {
			strncpy(s_music_album_text, tuple->value->cstring, sizeof(s_music_album_text));
			music_updated = true;
      		break;
		}
    	case ARTIST_KEY: {
			strncpy(s_music_artist_text, tuple->value->cstring, sizeof(s_music_artist_text));
			music_updated = true;
      		break;
		}
    	case SONG_KEY: {
			strncpy(s_music_song_text, tuple->value->cstring, sizeof(s_music_song_text));
			music_updated = true;
      		break;
		}
		case IS_PLAYING_KEY: {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "tuple: %s", (tuple->value->uint8 == 1) ? "true" : "false");
			s_music_is_playing = (tuple->value->uint8 == 1) ? true : false;
			music_updated = true;
		break;
		}
		case MESSAGE_SENDER_TO_SHOW: {
			sender = malloc(sizeof(char) * (strlen(tuple->value->cstring) + 1)); // +1 for \0
			strcpy(sender, tuple->value->cstring);
			break;
		}
    	case MESSAGE_SUBJECT_TO_SHOW: {
			subject = malloc(sizeof(char) * (strlen(tuple->value->cstring) + 1)); // +1 for \0
			strcpy(subject, tuple->value->cstring);
			break;
		}
    	case MESSAGE_BODY_TO_SHOW: {
     //   if (strlen(tuple->value->cstring) < 2000)
			int size = ((READER_BASE_SIZE * 40) + 1);
		    body = malloc(sizeof(char) * size); // +1 for \0
    //    else
//            body = (char*)malloc(2000 * sizeof(char));
//			if (body == NULL)
//				APP_LOG(APP_LOG_LEVEL_DEBUG, "body malloc is null");
			strncpy(body, tuple->value->cstring, size - 1);
			message_to_show = true;
			break;
		}
    	case MESSAGE_ACCOUNTID_TO_SHOW: {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "ACCOUNTID 1");
			accountid = malloc(sizeof(char) * (strlen(tuple->value->cstring) + 1)); // +1 for \0
			APP_LOG(APP_LOG_LEVEL_DEBUG, "2");
			strcpy(accountid, tuple->value->cstring);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "3");
			message_to_show = true;
			APP_LOG(APP_LOG_LEVEL_DEBUG, "ACCOUNTID 4");
			break;
		}
    	case MESSAGE_MESSAGEID_TO_SHOW: {
			messageid = malloc(sizeof(char) * (strlen(tuple->value->cstring) + 1)); // +1 for \0
			strcpy(messageid, tuple->value->cstring);
			message_to_show = true;
			break;
		}
    	case MESSAGE_TO_SHOW_IS_EMAIL: {
			is_email = (tuple->value->uint8 == 1);
			break;
		}
    	case MESSAGE_TO_SHOW_IS_READ: {
			is_read = (tuple->value->uint8 == 1);
			break;
		}
		case MESSAGE_TO_SHOW_IS_NEW_MESSAGE: {
			is_new_message = true;
			break;
		}
		case CONFIG_KEY_VIBRATION_TYPE: {
			persist_write_int(CONFIG_KEY_VIBRATION_TYPE, tuple->value->uint8);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "CONFIG_KEY_VIBRATION_TYPE: %d", (int)persist_read_int(CONFIG_KEY_VIBRATION_TYPE));
			break;
		}
		case CONFIG_KEY_MAIN_COLOR: {
			main_color = GColorFromHEX(tuple->value->uint32);
			persist_write_int(CONFIG_KEY_MAIN_COLOR, main_color.argb);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "CONFIG_KEY_MAIN_COLOR: %d %i", (int)persist_read_int(CONFIG_KEY_MAIN_COLOR), main_color.argb);
			break;
		}
		case CONFIG_KEY_LARGE_FONT: {
			persist_write_bool(CONFIG_KEY_LARGE_FONT, (tuple->value->uint8 == 1));
			APP_LOG(APP_LOG_LEVEL_DEBUG, "CONFIG_KEY_LARGE_FONT: %d", (int)persist_read_int(CONFIG_KEY_LARGE_FONT));
			snprintf(s_about_message, sizeof(s_about_message), "Settings set successfully.");
			dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_SETTINGS, s_about_message, false);
			break;
		}

		case CONFIG_KEY_REPLY1:
		case CONFIG_KEY_REPLY2:
		case CONFIG_KEY_REPLY3:
		case CONFIG_KEY_REPLY4:
		case CONFIG_KEY_REPLY5:
		case CONFIG_KEY_REPLY6: {
			persist_write_string(tuple->key, tuple->value->cstring);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "CONFIG_KEY_REPLY%d: %s", ((int)tuple->key - (int)CONFIG_KEY_REPLY1 + 1), tuple->value->cstring);
			break;
		}
		case MESSAGE_UNIQUE_ID_TO_ACK: {
			//if (strcmp(s_last_message_unique_id_to_ack, tuple->value->cstring) == 0) {
				//message_unique_id_to_ack_already_acked = true;
			//}
			//message_unique_id_to_ack = malloc(sizeof(char) * (strlen(tuple->value->cstring) + 1)); // +1 for \0
			//strcpy(message_unique_id_to_ack, tuple->value->cstring);
			break;
		}	
		case MESSAGE_LIST_ENTRY_KEY1:
		case MESSAGE_LIST_ENTRY_KEY2:
		case MESSAGE_LIST_ENTRY_KEY3:
		case MESSAGE_LIST_ENTRY_KEY4:
		case MESSAGE_LIST_ENTRY_KEY5: {
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "tuple: %s", tuple->value->cstring);
			char* parts[7];
			uint8_t partcount = 0;

			parts[partcount++] = tuple->value->cstring;

			char* ptr = tuple->value->cstring;
			while(*ptr) { //check if the string is over
				if(*ptr == '\e') {
					*ptr = 0;
					parts[partcount++] = ptr + 1;
				}
				ptr++;
			}
      
     // for (int i = 0; i < 7; i++)
     //   APP_LOG(APP_LOG_LEVEL_DEBUG, "%d: %s", i, parts[i]);

			number_in_array = atoi(parts[6]);
			if ((number_in_array >= 0) && (number_in_array < MESSAGE_MAX_MENU_ITEMS)) {
				strncpy(message_array_of_sender[number_in_array], parts[0], sizeof(message_array_of_sender[number_in_array]));
	        	strncpy(message_array_of_subject[number_in_array], parts[1], sizeof(message_array_of_subject[number_in_array]));
	        	strcpy(message_array_of_accountid[number_in_array], parts[2]);
	        	strcpy(message_array_of_messageid[number_in_array], parts[3]);
	        	message_array_of_is_email[number_in_array] = (strcmp(parts[4], "1") == 0);
	        	message_array_of_is_read[number_in_array] = (strcmp(parts[5], "1") == 0);
				message_update_menu(number_in_array);
			}

			*ptr = 0;
			break;
		}    
	}
  	tuple = dict_read_next(iter);
  }

 // if (message_unique_id_to_ack)
	//send_cstring_to_bridge(ACK_THIS_MESSAGE_UNIQUE_ID, message_unique_id_to_ack);
	
  if (message_to_show) {
	//if (!message_unique_id_to_ack_already_acked) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "main.c, sender: %s", sender);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "main.c, subject: %s", subject);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "main.c, body: %s", body);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "main.c, accountid: %s", accountid);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "main.c, messageid: %s", messageid);
		reader_window_create(sender, subject, body, accountid, messageid, is_email, is_read);
		if (persist_read_int(CONFIG_KEY_BACKLIGHT) == BACKLIGHT_ENABLED)
			light_enable_interaction();
		if (is_new_message) {
			handle_vibration();
		}
	  
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "s_message_main_window is in stack? %s", window_stack_contains_window(s_main_window) ? "true" : "false");  
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", message_is_window_in_stack() ? "true" : "false");
	    if (launch_reason() == APP_LAUNCH_PHONE)
			  window_stack_remove(s_main_window, false);
	  
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	//}
  }

  if ((music_updated) && (music_window_stack_contains_music_window())) {
	  music_update_all_text_layer();
  }
	
  free(sender);
  free(subject);
  free(body);
  free(accountid);
  free(messageid);
  tuple = NULL;
  free(tuple);
}

#ifdef PBL_MICROPHONE
void talk_callback(char *transcription, void *context) {
	send_cstring_to_bridge(TAKE_NOTE_COMMAND_KEY, transcription);
//	dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Note added to Remember successfully!", true);
}
#endif

void tap_handler(AccelAxisType axis, int32_t direction) {
	s_light_on = !s_light_on;
	light_enable(s_light_on);
}

static void menu_select_callback(int index, void *ctx) {
	check_heap_remaining();
	#ifndef PBL_MICROPHONE
		if (index > 3) 
			index += 1; // We have to adjust manually the index for Pebble that doesn't have the Voice option to keep it consistant
	#endif

	#ifdef PBL_PLATFORM_APLITE
		if (index > 4) 
			index += 1; //Aplite doesn't have enough memory for Settings, we have to adjust manually the index to keep it consistant
	#endif
	
	switch(index) {
		case 0: {
			bool email = true;
			message_init(email);
//			reader_window_create("Sender", "Subject", "Body really really lllllllllllllL l llll lllllll llll llll lllll ll llllllll llong", "123", "456", true, false);
			
			if (!s_wa_initialized)
				s_update_request_before_initialized = UPDATE_EMAIL_LIST;
			break;
		}
		case 1: {
			bool email = false;
			message_init(email);
			
			if (!s_wa_initialized)
				s_update_request_before_initialized = UPDATE_SMS_LIST;
			break;
		}
		case 2: {
			music_init();
			//window_stack_remove(s_main_window, false);
			//check_heap_remaining();
//			reader_window_create("Sender", "Subject", "Body really really lllllllllllllL l llll lllllll llll llll lllll ll llllllll llong", "123", "456", true, false);
			if (!s_wa_initialized)
				s_update_request_before_initialized = UPDATE_MUSIC_METADATA;
			break;
		}
		case 3: {
			doo_menu_window_push();
			break;
		}
		#ifdef PBL_MICROPHONE
		case 4: {
			talk_init("Take short Remember notes using voice technology.\nPress SELECT to start.", talk_callback, NULL);
			break;
		}
		#endif
		#ifndef PBL_PLATFORM_APLITE
		case 5: {
			// Settings menu option
			list_message_window_push();
			break;
		}
		#endif
		case 6: {
			snprintf(s_about_message, sizeof(s_about_message), "Bridge WA %s\nBridge API %s\n\nMemory %i\nInbound %i", 
					 version_get_version(),
					 BRIDGE_API_VERSION, 
					 (int)heap_bytes_free(), 
					 s_inbound_size);
			dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, s_about_message, false);
			break;
		}
		/*
		#ifdef PBL_PLATFORM_APLITE
		case 4: {
			s_vibe++;
			if (s_vibe > 3)
				s_vibe = 0;
			persist_write_int(CONFIG_KEY_VIBRATION_TYPE, s_vibe);
			snprintf(s_vibration_text, sizeof(s_vibration_text), "Vibration: %d", s_vibe);
			SimpleMenuItem *item = &s_first_menu_items[num_a_items - 1];
			item->title = s_vibration_text;
			layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
			APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", s_vibration_text);
			break;
		}
		#endif
		*/
		default: break;
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "menu_select_callback(%d) done", index);
	check_heap_remaining();
}

static void main_window_load(Window *window) {
  // Although we already defined NUM_FIRST_MENU_ITEMS, you can define
  // an int as such to easily change the order of menu items later

  num_a_items = 0;
  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ);
  #endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
    .title = "Email List",
	#ifndef PBL_PLATFORM_APLITE
  	.icon = s_menu_icon_image[num_a_items],
	#endif
    .callback = menu_select_callback,
  };
  num_a_items++;
	
  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_SMS_READ);
  #endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
    .title = "SMS List",
	#ifndef PBL_PLATFORM_APLITE
  	.icon = s_menu_icon_image[num_a_items],
	#endif
    .callback = menu_select_callback,
  };
  num_a_items++;
	
  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_MUSIC);
  #endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
    .title = "Music Control",
    #ifndef PBL_PLATFORM_APLITE
    .icon = s_menu_icon_image[num_a_items],
	#endif
    .callback = menu_select_callback,
  };
  num_a_items++;
	
  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_DOO);
  #endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
    .title = "doo",
    #ifndef PBL_PLATFORM_APLITE
    .icon = s_menu_icon_image[num_a_items],
	#endif
    .callback = menu_select_callback,
  };
  num_a_items++;

  #ifdef PBL_MICROPHONE
  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_TALK);
  #endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
    .title = "Voice To Note",
	#ifndef PBL_PLATFORM_APLITE
  	.icon = s_menu_icon_image[num_a_items],
	#endif
    .callback = menu_select_callback,
  };
  num_a_items++;
  #endif
	
  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_SETTINGS);
  //#endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
		.title = "Settings",
		//#ifndef PBL_PLATFORM_APLITE
  		.icon = s_menu_icon_image[num_a_items],
		//#endif
		.callback = menu_select_callback			
	};
  num_a_items++;
  #endif

  #ifndef PBL_PLATFORM_APLITE
  s_menu_icon_image[num_a_items] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ABOUT);
  #endif
  s_first_menu_items[num_a_items] = (SimpleMenuItem) {
		.title = "About",
	    #ifndef PBL_PLATFORM_APLITE
  		.icon = s_menu_icon_image[num_a_items],
	    #endif
		.callback = menu_select_callback			
	};
	num_a_items++;
	
	/*
	#ifdef PBL_PLATFORM_APLITE
	if (!persist_exists(CONFIG_KEY_VIBRATION_TYPE)) {
		s_vibe = 2;
		persist_write_int(CONFIG_KEY_VIBRATION_TYPE, s_vibe);
	}
	else 
		s_vibe = persist_read_int(CONFIG_KEY_VIBRATION_TYPE);
	snprintf(s_vibration_text, sizeof(s_vibration_text), "Vibration: %d", s_vibe);
	s_first_menu_items[num_a_items] = (SimpleMenuItem) {
		.title = s_vibration_text,
		.callback = menu_select_callback
	};
	num_a_items++;
	#endif
	*/

  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = num_a_items,
    .items = s_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, (launch_reason() == 2) ? main_color : disconnected_color, text_color);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
	
  //Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  bounds.origin.y = STATUS_BAR_LAYER_HEIGHT;

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
//  menu_layer_set_normal_colors(simple_menu_layer_get_menu_layer(s_simple_menu_layer), PBL_IF_BW_ELSE(GColorWhite, main_color), PBL_IF_BW_ELSE(GColorBlack, main_color));
  menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_simple_menu_layer), PBL_IF_BW_ELSE(GColorBlack, main_color), PBL_IF_BW_ELSE(GColorWhite, text_color));
}

static void main_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
  status_bar_layer_destroy(s_status_bar);
  #ifndef PBL_PLATFORM_APLITE
  for (uint8_t i = 0; i < MAX_MENU_ITEMS; i++) {
	  if (s_menu_icon_image[i])
  		gbitmap_destroy(s_menu_icon_image[i]);
  }  
  #endif
}

static void main_window_appear(Window *window) {
  if (s_status_bar) {
	status_bar_layer_set_colors(s_status_bar, (launch_reason() == 2) ? main_color : disconnected_color, text_color);
	send_uint8_to_bridge(IS_BRIDGE_LISTENING_KEY, 1);
  }
  if (s_simple_menu_layer)
   	menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_simple_menu_layer), main_color, text_color);
}

static void init() {
  check_heap_remaining();
  AppMessageResult result = 2;
  #ifdef PBL_PLATFORM_APLITE 
	s_inbound_size = 1500;
  #else
	s_inbound_size = 5000;
  #endif
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  while ((result != 0) && (s_inbound_size > 0)) {
	result = app_message_open(s_inbound_size, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_open() result: %d inbound_size: %d", (int)result, s_inbound_size);
	  if (result != 0) { s_inbound_size -= 500; }
  }

  disconnected_color = PBL_IF_COLOR_ELSE(GColorCyan, GColorLightGray);
  if (!persist_exists(CONFIG_KEY_MAIN_COLOR)) {
    persist_write_int(CONFIG_KEY_MAIN_COLOR, GColorFashionMagenta.argb);
    main_color = PBL_IF_COLOR_ELSE(GColorFashionMagenta, GColorBlack);
  }
  else {
	  uint8_t argb = persist_read_int(CONFIG_KEY_MAIN_COLOR);
	  if (argb < 192) {
		  argb = GColorFashionMagenta.argb;
		  persist_write_int(CONFIG_KEY_MAIN_COLOR, argb);
    }
    main_color.argb = argb;  
//	  APP_LOG(APP_LOG_LEVEL_DEBUG, "main_color.argb: %d", argb);
  }
//  text_color = PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite);	
  #ifdef PBL_BW
    main_color = GColorBlack;
  #endif
  text_color = PBL_IF_BW_ELSE(GColorWhite, GColorBlack);	
	
	//  message_initialize_menu_items();
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
	.appear = main_window_appear,
  });

  window_stack_push(s_main_window, true);
  if (persist_read_int(CONFIG_KEY_BACKLIGHT) == BACKLIGHT_ENABLED) {
  	light_enable_interaction();
  	s_light_on = true;
  }
  else {
  	s_light_on = false;
  }
  accel_tap_service_subscribe(&tap_handler);

  send_outbox_begin();
  send_dict_write_uint8(WATCHAPP_STARTED, 1);
	char inbound[] = "xxxx";
	snprintf(inbound, sizeof(inbound), "%i", s_inbound_size);
	send_dict_write_cstring(INBOUND_SIZE, inbound);
//  send_dict_write_uint8(INBOUND_SIZE, s_inbound_size);
  char api[] = "xx.xx";
  snprintf(api, sizeof(api), "%s", BRIDGE_API_VERSION);
  send_dict_write_cstring(IS_BRIDGE_LISTENING_KEY, api);
  if (!persist_exists(CONFIG_KEY_REPLY1))
	send_dict_write_uint8(UPDATE_SETTINGS_KEY, 1);
  send_outbox_send();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Launch reason: %d", launch_reason());
}

static void deinit() {
//	send_uint8_to_bridge(WATCHAPP_STOPPED, 1);
  light_enable_interaction();
  accel_tap_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}