#if !defined (LC_H)
#define LC_H

#include "lc_shared.h"

// Forward declarations
struct lc_memory;

enum input_modifier { M_CTRL = 0x01, M_SHIFT = 0x02, M_ALT = 0x04, M_CAPS = 0x08, M_NONE = 0x00 };

struct lc_input {
	int key;
	int modifier;
};

struct lc_app {
	lc_color current_color;
};

void app_init (lc_memory* memory);
void app_update (lc_memory* memory, lc_input input, int width, int height);

#endif 