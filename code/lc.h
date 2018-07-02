#if !defined (LC_H)
#define LC_H

#include "lc_shared.h"

// Forward declarations
struct lc_memory;

struct lc_app {
	lc_color current_color;
};

void app_update (lc_memory* memory, int width, int height);

#endif 