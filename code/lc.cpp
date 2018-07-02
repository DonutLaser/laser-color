#include "lc.h"

#include "lc_shared.h"
#include "lc_memory.h"
#include "lc_opengl.h"

#define clamp_value(x, min, max) (x < min) ? min : ((x > max) ? max : x)

#define PADDING 5
#define CLEAR_COLOR 200, 200, 200
#define DEFAULT_COLOR 255, 0, 0

#define KEY_COMMA 	0xBC
#define KEY_POINT 	0xBE
#define KEY_R		0x52
#define KEY_G		0x47
#define KEY_B		0x42

#define SINGLE_STEP 0.00392f // 1/255
#define MEDIUM_STEP 0.0392f  // 10/255

enum change_direction { D_INCREASE = 1, D_DECREASE = -1 };

static void change_color_component_value (float* component, float amount, change_direction direction) {
	*component = clamp_value (*component + ((int)direction * amount), 0.0f, 1.0f);
}

static void handle_input (lc_app* app, lc_input input) {
	switch (input.key) {
		case KEY_R: {
			app -> current_component = CC_R;
			break;
		}
		case KEY_G: {
			app -> current_component = CC_G;
			break;
		}
		case KEY_B: {
			app -> current_component = CC_B;
			break;
		}
		case KEY_COMMA: {
			if (app -> current_component == CC_R) {
				change_color_component_value (&app -> current_color.r,
											  (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP),
											  D_DECREASE);
			}
			else if (app -> current_component == CC_G) {
				change_color_component_value (&app -> current_color.g,
											  (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP),
											  D_DECREASE);
			}
			else if (app -> current_component == CC_B) {
				change_color_component_value (&app -> current_color.b,
											  (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP),
											  D_DECREASE);
			}
			break;
		}
		case KEY_POINT: {
			if (app -> current_component == CC_R) {
				change_color_component_value (&app -> current_color.r,
											  (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP),
											  D_INCREASE);
			}
			else if (app -> current_component == CC_G) {
				change_color_component_value (&app -> current_color.g,
											  (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP),
											  D_INCREASE);
			}
			else if (app -> current_component == CC_B) {
				change_color_component_value (&app -> current_color.b,
											  (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP),
											  D_INCREASE);
			}
			break;
		}
	}
}

void app_init (lc_memory* memory, int client_width, int client_height) {
	lc_app* app = (lc_app*)memory -> storage;
	app -> current_color = make_colorb (DEFAULT_COLOR);
	app -> current_component = CC_R;

	app -> client_width = client_width;
	app -> client_height = client_height;
}

void app_update (lc_memory* memory, lc_input input) {
	lc_app* app = (lc_app*)memory -> storage;

	handle_input (app, input);

	lc_color clear_color = make_colorb (CLEAR_COLOR);
	opengl_clear (app -> client_width, app -> client_height, clear_color);

	// (0,0) is bottom left, (width, height) top right 
	lc_rect color_rect = { };
	color_rect.x = PADDING;
	color_rect.y = app -> client_height - PADDING;
	color_rect.width = app -> client_width - PADDING;
	color_rect.height = (app -> client_height - (app -> client_height / 3)) - PADDING;

	lc_color color = app -> current_color;
	opengl_rect (color_rect, color);
}
