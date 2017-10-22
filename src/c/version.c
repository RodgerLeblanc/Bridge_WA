#include <pebble.h>
#include "version.h"

static char versionBuffer[10];

/* 
 * using the hacky method described here
 * https://forums.getpebble.com/discussion/10405/how-can-i-get-my-app-version-in-c-code
 */ 
#include "pebble_process_info.h"
extern const PebbleProcessInfo __pbl_app_info;

void prv_version_init() {
    snprintf(versionBuffer,sizeof(versionBuffer),"%d.%d",
        __pbl_app_info.process_version.major, 
        __pbl_app_info.process_version.minor
    );
}

char* version_get_version() {
	if (strcmp(versionBuffer, "") == 0)
		prv_version_init();
	
    return versionBuffer;
}