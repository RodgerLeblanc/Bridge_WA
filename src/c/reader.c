#include "commons.h"
#include "dialog_message_window.h"
#include "heap_check.h"
#include "reader.h"
#include "send_to_bridge.h"
#ifndef PBL_PLATFORM_APLITE
 // #include "dialog_message_window.h"
  #include "talk_to_pebble.h"
#endif

typedef struct {
	char sender[READER_BASE_SIZE];
	char subject[READER_BASE_SIZE];
	char body[READER_BASE_SIZE * 40];
	char accountid[15];
	char messageid[10];
	bool is_email;
	bool is_read;
	Window *window;
	ScrollLayer *scroll_layer;
	TextLayer *sender_layer;
	TextLayer *subject_layer;
	TextLayer *body_layer;
	#ifndef PBL_PLATFORM_APLITE
	BitmapLayer *icon_layer;
	GBitmap *icon_bitmap;
	BitmapLayer *menu_indicator_layer;
	GBitmap *menu_indicator_bitmap;
	#endif
	ActionMenu *action_menu;
	ActionMenuLevel *root_level;
	#ifndef PBL_PLATFORM_APLITE
	ActionMenuLevel *reply_level;
	#endif
	AppTimer *app_timer;
	StatusBarLayer *status_bar;
} ReaderWindow;

#ifndef PBL_PLATFORM_APLITE
static char reply[6][100];
#endif

typedef enum {
	VoiceReplyAction,
	MarkReadAction,
	DeleteMessageAction,
	Reply1Action,
	Reply2Action,
	Reply3Action,
	Reply4Action,
	Reply5Action,
	Reply6Action
} ReplyType;

/*
#ifdef PBL_PLATFORM_BASALT
static void animate_layer(Layer *layer, GRect start, GRect finish) {
  PropertyAnimation *prop_anim = property_animation_create_layer_frame(layer, &start, &finish);
  animation_set_duration((Animation*)prop_anim, 1000);
  animation_schedule((Animation*)prop_anim);
}
#endif
*/

void reader_select_click_handler(ClickRecognizerRef recognizer, void *context) {
	ReaderWindow * rw = (ReaderWindow*)context;
	// Configure the ActionMenu Window about to be shown
    ActionMenuConfig config = (ActionMenuConfig) {
		.root_level = rw->root_level,
	    .context = rw,
		.colors = {
          .background = PBL_IF_BW_ELSE(GColorBlack, main_color),
          .foreground = PBL_IF_BW_ELSE(GColorWhite, GColorBlack),
		},
		.align = ActionMenuAlignCenter
    };

    // Show the ActionMenu
    rw->action_menu = action_menu_open(&config);
}
//#endif

void reader_timeout(void *data) {
	ReaderWindow *rw = (ReaderWindow*)data;
	window_stack_remove(rw->window, true);
}

void reader_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "UP");
	scroll_layer_scroll_up_click_handler(recognizer, context);
	ReaderWindow *rw = (ReaderWindow*)context;
	//GPoint offset = scroll_layer_get_content_offset(rw->scroll_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Offset: %d %d", offset.x, offset.y);
	if(rw->app_timer) {
		app_timer_cancel(rw->app_timer);
		rw->app_timer = NULL;
		rw->app_timer = app_timer_register(120000, reader_timeout, rw); // 2 minutes
	}
}

void reader_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "DOWN");
	scroll_layer_scroll_down_click_handler(recognizer, context);
	ReaderWindow *rw = (ReaderWindow*)context;
	if (rw == NULL) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "rw is NULL");
		return;
	}
	//GPoint offset = scroll_layer_get_content_offset(rw->scroll_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Offset: %d %d", (int)offset.x, (int)offset.y);
	if(rw->app_timer) {
		app_timer_cancel(rw->app_timer);
		rw->app_timer = NULL;
		rw->app_timer = app_timer_register(120000, reader_timeout, rw); // 2 minutes
	}
}

void reader_click_config_provider(void *context) {
	//#ifndef PBL_PLATFORM_APLITE
	window_single_click_subscribe(BUTTON_ID_SELECT, reader_select_click_handler);
	//#endif
	//window_single_click_subscribe(BUTTON_ID_UP, reader_up_click_handler);
	//window_single_click_subscribe(BUTTON_ID_DOWN, reader_down_click_handler);
}

//#ifndef PBL_PLATFORM_APLITE
bool reader_check_action_validity(ReaderWindow *rw) {
	return ((atoi(rw->accountid) > 0) && (atoi(rw->messageid) > 0));
}

#ifdef PBL_MICROPHONE
void reader_talk_callback(char *transcription, void *context) {
	ReaderWindow *rw = (ReaderWindow*)context;
	send_outbox_begin();
	send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, transcription);
	send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
	send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
	send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
	send_outbox_send();
	
	if (reader_check_action_validity(rw))
	    dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
	else
		dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while replying, reload message and try again.", true);
}
#endif

void action_performed_callback(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
    // An action that was created with this callback assigned was chosen
	ReaderWindow * rw = (ReaderWindow*)context;
    ReplyType type = (ReplyType)action_menu_item_get_action_data(action);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Type: %d", (int)type);
	switch (type) {
		case VoiceReplyAction: {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "VoiceReplyAction");
			#ifdef PBL_MICROPHONE
	 			talk_init("", reader_talk_callback, rw);
			#endif
			break;
		}
		#ifndef PBL_PLATFORM_APLITE
		case Reply1Action: {
			send_outbox_begin();
			send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, reply[0]);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();

			if (reader_check_action_validity(rw))
			    dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
			else
			    dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while replying, reload message and try again.", true);
			break;
		}
		case Reply2Action: {
			send_outbox_begin();
			send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, reply[1]);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			
			if (reader_check_action_validity(rw))
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
			else
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while sending reply, reload message and try again.", true);
			break;
		}
		case Reply3Action: {
			send_outbox_begin();
			send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, reply[2]);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			
			if (reader_check_action_validity(rw))
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
			else
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while sending reply, reload message and try again.", true);
			break;
		}
		case Reply4Action: {
			send_outbox_begin();
			send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, reply[3]);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			
			if (reader_check_action_validity(rw))
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
			else
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while sending reply, reload message and try again.", true);
			break;
		}
		case Reply5Action: {
			send_outbox_begin();
			send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, reply[4]);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			
			if (reader_check_action_validity(rw))
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
			else
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while sending reply, reload message and try again.", true);
			break;
		}
		case Reply6Action: {
			send_outbox_begin();
			send_dict_write_cstring(READER_VOICEREPLY_COMMAND_KEY, reply[5]);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			
			if (reader_check_action_validity(rw))
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Reply sent successfully!", true);
			else
				dialog_message_show(RESOURCE_ID_IMAGE_PEBBLE_TALK, "Error while sending reply, reload message and try again.", true);
			break;
		}
		#endif
		case MarkReadAction: {
			send_outbox_begin();
			send_dict_write_uint8(READER_MARKREAD_COMMAND_KEY, 1);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			rw->is_read = true;
			#ifndef PBL_PLATFORM_APLITE
			rw->icon_bitmap = gbitmap_create_with_resource(rw->is_email ? (rw->is_read ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_EMAIL_UNREAD) : (rw->is_read ? RESOURCE_ID_IMAGE_PEBBLE_SMS_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_UNREAD));
    		bitmap_layer_set_bitmap(rw->icon_layer, rw->icon_bitmap);
			#endif
			
			if (reader_check_action_validity(rw))
				dialog_message_show(rw->is_email ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_READ, "Message marked read successfully!", true);
			else
				dialog_message_show(rw->is_email ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_READ, "Error while marking read, reload message and try again.", true);
			break;
		}
		case DeleteMessageAction: {
			send_outbox_begin();
			send_dict_write_uint8(READER_DELETE_COMMAND_KEY, 1);
			send_dict_write_cstring(MESSAGE_ACCOUNTID_KEY, rw->accountid);
			send_dict_write_cstring(MESSAGE_MESSAGEID_KEY, rw->messageid);
			send_dict_write_uint8(MESSAGE_IS_EMAIL_KEY, rw->is_email ? 1 : 0);
			send_outbox_send();
			
//			APP_LOG(APP_LOG_LEVEL_DEBUG, "message_is_window_in_stack? %s", message_is_window_in_stack() ? "true" : "false");
			//if (message_is_window_in_stack())
				//bridge_ask_to_send_data(true, false);
			
			if (reader_check_action_validity(rw))
				dialog_message_show(rw->is_email ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_READ, "Message deleted successfully!", true);
			else
				dialog_message_show(rw->is_email ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_READ, "Error while deleting message, reload message and try again.", true);
			window_stack_remove(rw->window, true);
			break;
		}
		default: {
			break;
		}
	}
}
//#endif

// Setup the scroll layer on window appear
// We do this here in order to be able to get the max used text size
void reader_window_appear(Window *window) {
	ReaderWindow * rw = (ReaderWindow*)window_get_user_data(window);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "body appeared: %s", rw->body);
	check_heap_remaining();
}

void reader_window_load(Window *window) {
	ReaderWindow *rw = (ReaderWindow*)window_get_user_data(window);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "body loaded: %s", rw->body);
	Layer *window_layer = window_get_root_layer(rw->window);

	rw->status_bar = status_bar_layer_create();
	#ifdef PBL_COLOR
	status_bar_layer_set_colors(rw->status_bar, main_color, GColorBlack);
	#endif
    layer_add_child(window_layer, status_bar_layer_get_layer(rw->status_bar));
	
	GRect bounds = layer_get_frame(window_layer);
	bounds.origin.y += STATUS_BAR_LAYER_HEIGHT;
	bounds.size.h -= STATUS_BAR_LAYER_HEIGHT;
	//int max_height = 2000;

	// Initialize the scroll layer
	rw->scroll_layer = scroll_layer_create(bounds);
	//GSize slcs = scroll_layer_get_content_size(rw->scroll_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "slcs1: %d %d", slcs.w, slcs.h);

	// This binds the scroll layer to the window so that up and down map to scrolling
	// You may use scroll_layer_set_callbacks to add or override interactivity
	scroll_layer_set_context(rw->scroll_layer, rw);
	scroll_layer_set_callbacks(rw->scroll_layer, (ScrollLayerCallbacks) {
		.click_config_provider = &reader_click_config_provider
	});
	scroll_layer_set_click_config_onto_window(rw->scroll_layer, window);

	#ifdef PBL_ROUND
	scroll_layer_set_paging(rw->scroll_layer, true);
	#endif

	int y = 0;
	int object_height;
	
	if (!persist_exists(CONFIG_KEY_LARGE_FONT))
		persist_write_bool(CONFIG_KEY_LARGE_FONT, false);
	
	bool large_font = persist_read_bool(CONFIG_KEY_LARGE_FONT);
	
	int max_height = (large_font ? 3000 : 2000);
	
	#ifndef PBL_PLATFORM_APLITE
	y += 3;
	object_height = 18;
	rw->icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - (25 / 2), y, 25, object_height));
	y += object_height;
	bitmap_layer_set_compositing_mode(rw->icon_layer, GCompOpSet); // Allow transparent images
	rw->icon_bitmap = gbitmap_create_with_resource(rw->is_email ? (rw->is_read ? RESOURCE_ID_IMAGE_PEBBLE_EMAIL_READ : RESOURCE_ID_IMAGE_PEBBLE_EMAIL_UNREAD) : (rw->is_read ? RESOURCE_ID_IMAGE_PEBBLE_SMS_READ : RESOURCE_ID_IMAGE_PEBBLE_SMS_UNREAD));
    bitmap_layer_set_bitmap(rw->icon_layer, rw->icon_bitmap);
	scroll_layer_add_child(rw->scroll_layer, bitmap_layer_get_layer(rw->icon_layer));
	#endif
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "sender y: %d", y);
	// Initialize the text layer
	object_height = 2 * (large_font ? 28 : 18) + 4;
	uint8_t sender_height = object_height;
	rw->sender_layer = text_layer_create(GRect(0, y, bounds.size.w, max_height - y));
	y += object_height;
	if (rw->sender_layer == NULL) APP_LOG(APP_LOG_LEVEL_DEBUG, "rw->sender_layer is NULL, heap: %d", (int)heap_bytes_free());
	text_layer_set_text(rw->sender_layer, rw->sender);
	text_layer_set_font(rw->sender_layer, fonts_get_system_font(large_font ? FONT_KEY_GOTHIC_28 : FONT_KEY_GOTHIC_18));
	if (!rw->is_email) {
		text_layer_set_background_color(rw->sender_layer, main_color);
		#ifdef PBL_BW
		text_layer_set_text_color(rw->sender_layer, GColorWhite);
		#endif
	}
	text_layer_set_text_alignment(rw->sender_layer, GTextAlignmentCenter);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "subject y: %d", y);
	rw->subject[sizeof(rw->subject)-1] = '\0';
	object_height = rw->is_email ? 3 * (large_font ? 28 : 18) + 4 : 0;
	uint8_t subject_height = object_height;
	rw->subject_layer = text_layer_create(GRect(0, y, bounds.size.w, max_height - y));
	y += object_height;
	text_layer_set_text(rw->subject_layer, rw->subject);
	text_layer_set_font(rw->subject_layer, fonts_get_system_font(large_font ? FONT_KEY_GOTHIC_28 : FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_background_color(rw->subject_layer, main_color);
	#ifdef PBL_BW
	text_layer_set_text_color(rw->subject_layer, GColorWhite);
	#endif
	text_layer_set_text_alignment(rw->subject_layer, GTextAlignmentCenter);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "body y: %d", y);
	rw->body_layer = text_layer_create(GRect(0, y, bounds.size.w, max_height - y));
	text_layer_set_text(rw->body_layer, rw->body);
	text_layer_set_font(rw->body_layer, fonts_get_system_font(large_font ? FONT_KEY_GOTHIC_28_BOLD : FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(rw->body_layer, GTextAlignmentCenter);

	scroll_layer_add_child(rw->scroll_layer, text_layer_get_layer(rw->sender_layer));
	scroll_layer_add_child(rw->scroll_layer, text_layer_get_layer(rw->subject_layer));
	scroll_layer_add_child(rw->scroll_layer, text_layer_get_layer(rw->body_layer));

	layer_add_child(window_layer, scroll_layer_get_layer(rw->scroll_layer));

	#ifdef PBL_ROUND
	// Enable TextLayer text flow and paging with inset size 2px
	text_layer_enable_screen_text_flow_and_paging(rw->sender_layer, 2);
	text_layer_enable_screen_text_flow_and_paging(rw->subject_layer, 2);
	text_layer_enable_screen_text_flow_and_paging(rw->body_layer, 2);
	#endif

	// Trim text layer and scroll content to fit text box
//	GSize sender_size = text_layer_get_content_size(rw->sender_layer);
	text_layer_set_size(rw->sender_layer, GSize(bounds.size.w, sender_height));

//	GSize subject_size = text_layer_get_content_size(rw->subject_layer);
	text_layer_set_size(rw->subject_layer, GSize(bounds.size.w, subject_height));
										  
	/*
	#ifdef PBL_ROUND
		text_layer_enable_screen_text_flow_and_paging(rw->subject_layer, 2);
	#endif
	*/

	GSize body_size = text_layer_get_content_size(rw->body_layer);
	text_layer_set_size(rw->body_layer, GSize(bounds.size.w, body_size.h + 4));

	// This was used to animate text layers, not used anymore
	//GRect sender_finish_text_bounds = max_text_bounds;
	//sender_finish_text_bounds.origin.y = max_text_bounds.origin.y + 18 + 4;
	//animate_layer(text_layer_get_layer(rw->sender_layer), max_text_bounds, sender_finish_text_bounds);

	//GRect subject_finish_text_bounds = max_text_bounds;
	//subject_finish_text_bounds.origin.y = max_text_bounds.origin.y + 18 + 4 + sender_size.h + 4;
	//animate_layer(text_layer_get_layer(rw->subject_layer), max_text_bounds, subject_finish_text_bounds);

	//GRect body_finish_text_bounds = max_text_bounds;
//	int offset = 18 + 4 + sender_size.h + 4 + subject_size.h + (rw->is_email ? 4 : 0);
	//body_finish_text_bounds.origin.y = max_text_bounds.origin.y + offset;
	//body_finish_text_bounds.size.h = max_text_bounds.size.h + offset;
	//animate_layer(text_layer_get_layer(rw->body_layer), max_text_bounds, body_finish_text_bounds);

	// Set the ScrollLayer's content size to the total size of the text
	GSize body_content_size = text_layer_get_content_size(rw->body_layer);
	scroll_layer_set_content_size(rw->scroll_layer, GSize(bounds.size.w, STATUS_BAR_LAYER_HEIGHT + y + body_content_size.h + 4));
	//scroll_layer_set_frame(rw->scroll_layer, bounds);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "bounds: %d %d %d %d", bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h);

	#ifndef PBL_PLATFORM_APLITE
	GRect icon_rect = GRect(0, 0, 14, 14);
	grect_align(&icon_rect, &bounds, GAlignRight, false);
	rw->menu_indicator_layer = bitmap_layer_create(icon_rect);
	bitmap_layer_set_compositing_mode(rw->menu_indicator_layer, GCompOpSet);
	rw->menu_indicator_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_INDICATOR);
	bitmap_layer_set_bitmap(rw->menu_indicator_layer, rw->menu_indicator_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(rw->menu_indicator_layer));	
	#endif
	
	int number_of_root_actions = (rw->is_read ? 1 : 2);
	#ifndef PBL_PLATFORM_APLITE
	number_of_root_actions++; // Reply sub-menu
	#endif
	#ifdef PBL_MICROPHONE
	number_of_root_actions++; // Voice reply
	#endif
	rw->root_level = action_menu_level_create(number_of_root_actions);
	#ifndef PBL_PLATFORM_APLITE
	rw->reply_level = action_menu_level_create(6);
	action_menu_level_add_child(rw->root_level, rw->reply_level, "Reply");

	// Set up the secondary actions
	if (persist_exists(CONFIG_KEY_REPLY1)) {
		persist_read_string(CONFIG_KEY_REPLY1, reply[0], sizeof(reply[0]));
		if (strcmp(reply[0], "") == 0) {
			strcpy(reply[0], "Ok");
			persist_write_string(CONFIG_KEY_REPLY1, reply[0]);
		}
	}
	else {
		strcpy(reply[0], "Ok");
		persist_write_string(CONFIG_KEY_REPLY1, reply[0]);
	}

	if (persist_exists(CONFIG_KEY_REPLY2)) {
		persist_read_string(CONFIG_KEY_REPLY2, reply[1], sizeof(reply[1]));
		if (strcmp(reply[1], "") == 0) {
			strcpy(reply[1], "Yes");
			persist_write_string(CONFIG_KEY_REPLY2, reply[1]);
		}
	}
	else {
		strcpy(reply[1], "Yes");
		persist_write_string(CONFIG_KEY_REPLY2, reply[1]);
	}

	if (persist_exists(CONFIG_KEY_REPLY3)) {
		persist_read_string(CONFIG_KEY_REPLY3, reply[2], sizeof(reply[2]));
		if (strcmp(reply[2], "") == 0) {
			strcpy(reply[2], "No");
			persist_write_string(CONFIG_KEY_REPLY3, reply[2]);
		}
	}
	else {
		strcpy(reply[2], "No");
		persist_write_string(CONFIG_KEY_REPLY3, reply[2]);
	}

	if (persist_exists(CONFIG_KEY_REPLY4)) {
		persist_read_string(CONFIG_KEY_REPLY4, reply[3], sizeof(reply[3]));
		if (strcmp(reply[3], "") == 0) {
			strcpy(reply[3], "Talk to you later");
			persist_write_string(CONFIG_KEY_REPLY4, reply[3]);
		}
	}
	else {
		strcpy(reply[3], "Talk to you later");
		persist_write_string(CONFIG_KEY_REPLY4, reply[3]);
	}

	if (persist_exists(CONFIG_KEY_REPLY5)) {
		persist_read_string(CONFIG_KEY_REPLY5, reply[4], sizeof(reply[4]));
		if (strcmp(reply[4], "") == 0) {
			strcpy(reply[4], "Will be late");
			persist_write_string(CONFIG_KEY_REPLY5, reply[4]);
		}
	}
	else {
		strcpy(reply[4], "Will be late");
		persist_write_string(CONFIG_KEY_REPLY5, reply[4]);
	}

	if (persist_exists(CONFIG_KEY_REPLY6)) {
		persist_read_string(CONFIG_KEY_REPLY6, reply[5], sizeof(reply[5]));
		if (strcmp(reply[5], "") == 0) {
			strcpy(reply[5], "I'm on Pebble, will chat later");
			persist_write_string(CONFIG_KEY_REPLY6, reply[5]);
		}
	}
	else {
		strcpy(reply[5], "I'm on Pebble, will chat later");
		persist_write_string(CONFIG_KEY_REPLY6, reply[5]);
	}

	action_menu_level_add_action(rw->reply_level, reply[0], action_performed_callback, (void *)Reply1Action);
	action_menu_level_add_action(rw->reply_level, reply[1], action_performed_callback, (void *)Reply2Action);
	action_menu_level_add_action(rw->reply_level, reply[2], action_performed_callback, (void *)Reply3Action);
	action_menu_level_add_action(rw->reply_level, reply[3], action_performed_callback, (void *)Reply4Action);
	action_menu_level_add_action(rw->reply_level, reply[4], action_performed_callback, (void *)Reply5Action);
	action_menu_level_add_action(rw->reply_level, reply[5], action_performed_callback, (void *)Reply6Action);
	#endif

	#ifdef PBL_MICROPHONE
	 	action_menu_level_add_action(rw->root_level, "Voice Reply", action_performed_callback, (void *)VoiceReplyAction);
	#endif
	if (!rw->is_read)
	    action_menu_level_add_action(rw->root_level, "Mark Read", action_performed_callback, (void *)MarkReadAction);
	
	action_menu_level_add_action(rw->root_level, "Delete Message", action_performed_callback, (void *)DeleteMessageAction);
	//#endif
	
	rw->app_timer = app_timer_register(120000, reader_timeout, rw); // 2 minutes
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Is top window? %d", (rw->window == window_stack_get_top_window()) ? 1 : 0);
}

void reader_window_unload(Window *window) {
	ReaderWindow *rw = (ReaderWindow*)window_get_user_data(window);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "body unloaded: %s", rw->body);

	layer_remove_from_parent(scroll_layer_get_layer(rw->scroll_layer));
	scroll_layer_destroy(rw->scroll_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 1");
	text_layer_destroy(rw->sender_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 2");
	text_layer_destroy(rw->subject_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 3");
	text_layer_destroy(rw->body_layer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 4");
	app_timer_cancel(rw->app_timer);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 5");
	rw->app_timer = NULL;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 6");
	#ifndef PBL_PLATFORM_APLITE
	//action_menu_hierarchy_destroy(rw->reply_level, NULL, NULL);
	#endif
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 7");
	action_menu_hierarchy_destroy(rw->root_level, NULL, NULL);
	//#endif
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 1");
	
	#ifndef PBL_PLATFORM_APLITE
	if (rw->menu_indicator_bitmap) {
		gbitmap_destroy(rw->menu_indicator_bitmap);
	}
	bitmap_layer_destroy(rw->menu_indicator_layer);

	if (rw->icon_bitmap) {
    	gbitmap_destroy(rw->icon_bitmap);
	}
	bitmap_layer_destroy(rw->icon_layer);
	#endif
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 2");
	
	status_bar_layer_destroy(rw->status_bar);

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 3");
	
	free(rw);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 4");
	
	window_destroy(window);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CRASH 5");
	
	if (window_stack_get_top_window() == NULL) {
		send_uint8_to_bridge(WATCHAPP_STOPPED, 1);
	}
}

void reader_window_create(char *sender, char *subject, char *body, char *accountid, char *messageid, bool is_email, bool is_read) {
	#ifdef PBL_PLATFORM_APLITE
	ReaderWindow *test = (ReaderWindow*)window_get_user_data(window_stack_get_top_window());
	if (test != NULL) {
		window_stack_pop(false);
	}
	else {
		free(test);
	}
	#endif
	ReaderWindow * rw = malloc(sizeof(ReaderWindow));
	strcpy(rw->sender, sender);
	
	if (strlen(subject) == 0)
		strcpy(rw->subject, "[EMPTY SUBJECT]");
	else	
	    strcpy(rw->subject, subject);
	
	strncpy(rw->body, body, sizeof(rw->body));
	strncpy(rw->accountid, accountid, sizeof(rw->accountid));
	strncpy(rw->messageid, messageid, sizeof(rw->messageid));
	rw->is_email = is_email;
	rw->is_read = is_read;
//	APP_LOG(APP_LOG_LEVEL_DEBUG, "reader_window_create, subject: %s\nbody: %s", rw->subject, rw->body);
	
	rw->window = window_create();
	window_set_user_data(rw->window, rw);

	// Setup the window handlers
	window_set_window_handlers(rw->window, (WindowHandlers) {
		.appear = &reader_window_appear,
		.load = &reader_window_load,
		.unload = &reader_window_unload
	});

	window_stack_push(rw->window, true);
//	APP_LOG(APP_LOG_LEVEL_DEBUG, "reader_window_create() done");
	check_heap_remaining();
}