#if !defined (LC_H)
#define LC_H

#include "lc_shared.h"

// Forward declarations
struct lc_memory;

enum input_modifier { M_CTRL = 0x01, M_SHIFT = 0x02, M_ALT = 0x04, M_CAPS = 0x08, M_NONE = 0x00 };
enum color_component { CC_R, CC_G, CC_B };

struct lc_input {
	int key;
	int modifier;
};

struct lc_app {
	int client_width, client_height;
	lc_color current_color;
	color_component current_component;
};

void app_init (lc_memory* memory, int client_width, int client_height);
void app_update (lc_memory* memory, lc_input input);

#endif 