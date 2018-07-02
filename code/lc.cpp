#include "lc.h"

#include "lc_shared.h"
#include "lc_memory.h"
#include "lc_opengl.h"

#define PADDING 5
#define CLEAR_COLOR { 0.7f, 0.7f, 0.7f }
#define DEFAULT_COLOR { 1.0f, 0.0f, 0.0f }

void app_update (lc_memory* memory, int width, int height) {
	lc_app* app = (lc_app*)memory -> storage;

	lc_color clear_color = CLEAR_COLOR;
	opengl_clear (width, height, clear_color);

	// (0,0) is bottom left, (width, height) top right 
	lc_rect color_rect = { };
	color_rect.x = PADDING;
	color_rect.y = height - PADDING;
	color_rect.width = width - PADDING;
	color_rect.height = (height - (height / 3)) - PADDING;

	lc_color color = DEFAULT_COLOR;
	opengl_rect (color_rect, color);
}
