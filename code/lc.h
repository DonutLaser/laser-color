#if !defined (LC_H)
#define LC_H

#include "lc_shared.h"
#include "lc_platform.h"

#define PATH_MAX 128 

#define MAX_COLORS_IN_LIBRARY 11 

// Forward declarations
struct lc_memory;

enum input_modifier { M_CTRL = 0x01, M_SHIFT = 0x02, M_ALT = 0x04, M_CAPS = 0x08, M_NONE = 0x00 };

struct lc_file {
	void* handle;
	char* path;
};

struct lc_input {
	int key;
	int modifier;
};

struct lc_color_library {
	lc_color colors[MAX_COLORS_IN_LIBRARY];
	int count;
};

struct lc_app {
	int client_width, client_height;

	lc_color current_color;
	lc_color previous_color;
	float* current_component;
	int current_component_index;
	int current_swatch_index;

	lc_file color_library_file;
	lc_color_library color_swatches;
	bool color_library_is_dirty;

	platform_api platform;
};

void app_init (lc_memory* memory, platform_api platform, int client_width, int client_height, char* documents);
void app_update (lc_memory* memory, lc_input input);
void app_close (lc_memory* memory);

#endif 