#if !defined (LC_H)
#define LC_H

#include "lc_shared.h"
#include "lc_platform.h"
#include "lc_font.h"
#include "lc_status.h"

#define PATH_MAX 128 

#define MAX_COLORS_IN_LIBRARY 99 

// Forward declarations
struct lc_memory;

enum input_modifier { M_CTRL = 0x01, M_SHIFT = 0x02, M_ALT = 0x04, M_CAPS = 0x08, M_NONE = 0x00 };
enum mouse_state { LMB_DOWN = 0x01, LMB_UP = 0x02 };
enum ui_images { UI_SLIDER_ARROW_LEFT, UI_SLIDER_ARROW_RIGHT, UI_SWATCH_ARROW_TOP, UI_SWATCH_ARROW_BOTTOM, UI_CLOSE, UI_MINIMIZE, UI_COUNT };
enum fonts { FT_MAIN, FT_SMALL, FT_COUNT };

struct lc_file {
	void* handle;
	char* path;
};

struct lc_mouse {
	vector2 position;
	vector2 screen_position;
	int state;
};

struct lc_input {
	int key;
	int modifier;
	lc_mouse mouse;
};

struct lc_color_library {
	lc_color samples[MAX_COLORS_IN_LIBRARY];
	int count;
};

struct lc_app {
	vector2 client_size;

	lc_color current_color;
	lc_color previous_color;
	float* current_component;
	int current_component_index;
	int current_sample_index;
	int sample_bar_offset_target;
	int sample_bar_offset;
	int current_screen_sample_index;

	lc_file color_library_file;
	lc_color_library color_samples;
	bool color_library_is_dirty;

	status status_bar;

	lc_image ui_images[UI_COUNT];
	lc_font fonts[FT_COUNT];

	platform_api platform;
};

void app_init (lc_memory* memory, platform_api platform, vector2 client_size, char* documents, vector2* title_bar_size);
void app_update (lc_memory* memory, double delta_time, lc_input input);
void app_close (lc_memory* memory);

#endif 