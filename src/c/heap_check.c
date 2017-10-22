#include "pebble.h"
#include "heap_check.h"

/*
static char* test;
static int bytes_remaining = 1;

static void allocate_memory() {
	test = (char*)malloc(bytes_remaining * sizeof(char));
}

static void free_memory() {
	free(test);
	test = NULL;
}
*/
int check_heap_remaining() {
	/*
	int step = 1000;
	while (true) {
		allocate_memory();
		while (test != NULL) {
			free_memory();
			bytes_remaining += step;
			allocate_memory();
		}
		free_memory();
		bytes_remaining -= step;
		
		if(step == 1)
			break; // exit loop
		
		step = step / 10;
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap remaining: %d", bytes_remaining);
	return bytes_remaining;
	*/
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap remaining: %d", (int)heap_bytes_free());
	return (int)heap_bytes_free();
}