#include "lc.h"

#include "lc_shared.h"
#include "lc_memory.h"
#include "lc_opengl.h"

void app_update (lc_memory* memory, int width, int height) {
	lc_app* app = (lc_app*)memory -> storage;

	lc_color clear_color = { 0.0f, 0.0f, 0.0f };
	opengl_clear (width, height, clear_color);

	// (0,0) is bottom left, (width, height) top right 
	app -> test_rect = { };
	app -> test_rect.y = height;
	app -> test_rect.width = width;
	app -> test_rect.height = height - (height / 3);
	app -> test_color = { 0.0f, 0.1f, 0.4f }; 
	opengl_rect (app -> test_rect, app -> test_color);
}
