#pragma once

#include "pebble.h"

#define MESSAGE_NUM_MENU_SECTIONS 1
#ifdef PBL_PLATFORM_APLITE
  #define MESSAGE_MAX_MENU_ITEMS 10
#else
  #define MESSAGE_MAX_MENU_ITEMS 20
#endif

char message_array_of_sender[MESSAGE_MAX_MENU_ITEMS][50];
char message_array_of_subject[MESSAGE_MAX_MENU_ITEMS][100];
char message_array_of_accountid[MESSAGE_MAX_MENU_ITEMS][15];
char message_array_of_messageid[MESSAGE_MAX_MENU_ITEMS][10];
bool message_array_of_is_email[MESSAGE_MAX_MENU_ITEMS];
bool message_array_of_is_read[MESSAGE_MAX_MENU_ITEMS];

void message_initialize_menu_items();
void populate_menu_items();
void message_update_menu(uint8_t item_number);
void message_update_all_menu();
void message_main_window_unload(Window *window);
bool message_is_window_in_stack();
void message_init(bool send_email);