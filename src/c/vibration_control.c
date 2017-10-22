#include <pebble.h>
#include "vibration_control.h"
#include "commons.h"

enum VibrationType {
	VIBRATION_DISABLED,
	VIBRATION_LOW,
	VIBRATION_MEDIUM,
	VIBRATION_HIGH
};

static const uint32_t low_vibe_segments[] = { 150 };
VibePattern low_vibe = {
  .durations = low_vibe_segments,
  .num_segments = ARRAY_LENGTH(low_vibe_segments),
};

static const uint32_t medium_vibe_segments[] = { 300 };
VibePattern medium_vibe = {
  .durations = medium_vibe_segments,
  .num_segments = ARRAY_LENGTH(medium_vibe_segments),
};

static const uint32_t high_vibe_segments[] = { 450 };
VibePattern high_vibe = {
  .durations = high_vibe_segments,
  .num_segments = ARRAY_LENGTH(high_vibe_segments),
};

void handle_vibration() {
	if (quiet_time_is_active()) return;
	
	uint8_t key = CONFIG_KEY_VIBRATION_TYPE;
	if (!persist_exists(key))
		persist_write_int(key, VIBRATION_MEDIUM);
	
	enum VibrationType type = persist_read_int(key);
	switch(type) {
		case VIBRATION_DISABLED: break;
		case VIBRATION_LOW: {
			vibes_enqueue_custom_pattern(low_vibe);
			break;
		}
		case VIBRATION_MEDIUM: {
			vibes_enqueue_custom_pattern(medium_vibe);
			break;
		}
		case VIBRATION_HIGH: {
			vibes_enqueue_custom_pattern(high_vibe);
			break;
		}
	}
}