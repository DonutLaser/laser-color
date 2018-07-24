#include "lc.h"

#include "lc_platform.h"
#include "lc_shared.h"
#include "lc_memory.h"
#include "lc_opengl.h"

#define clamp_value(x, min, max) (x < min) ? min : ((x > max) ? max : x)

#define HORIZONTAL_PADDING 10 
#define VERTICAL_PADDING 5 
#define SLIDER_HEIGHT 15 
#define SLIDER_HANDLE_WIDTH 8
#define CLEAR_COLOR 200, 200, 200
#define DEFAULT_COLOR 255, 255, 255 
#define HIGHLIGHT_COLOR 160, 160, 160
#define HANDLE_COLOR 220, 220, 220

#define KEY_COMMA 	0xBC
#define KEY_POINT 	0xBE
#define KEY_J		0x4A
#define KEY_K		0x4B
#define KEY_ALPHA_0 0x30
#define KEY_ALPHA_4 0x34

#define SINGLE_STEP 0.00392f // 1/255
#define MEDIUM_STEP 0.0392f  // 10/255
#define FULL_STEP 1.0f

enum change_direction { D_INCREASE = 1, D_DECREASE = -1 };

static void change_color_component_value (lc_app* app, float amount, change_direction direction) {
	float* component;
	if (app -> current_component == CC_R)
		component = &app -> current_color.r;
	else if (app -> current_component == CC_G)
		component = &app -> current_color.g;
	else
		component = &app -> current_color.b;

	*component = clamp_value (*component + ((int)direction * amount), 0.0f, 1.0f);
}

static void handle_input (lc_app* app, lc_input input) {
	switch (input.key) {
		case KEY_J: {
			app -> current_component = (color_component)(clamp_value ((int)app -> current_component + 1, 0, CC_B));
			break;
		}
		case KEY_K: {
			app -> current_component = (color_component)(clamp_value (app -> current_component - 1, 0, CC_B));
			break;
		}
		case KEY_ALPHA_0: {
			change_color_component_value (app, FULL_STEP, D_DECREASE);
			break;
		}
		case KEY_ALPHA_4: {
			if (input.modifier & M_SHIFT)
				change_color_component_value (app, FULL_STEP, D_INCREASE);

			break;
		}
		case KEY_COMMA: {
			change_color_component_value (app, 
										 (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP), 
										 D_DECREASE);

			break;
		}
		case KEY_POINT: {
			change_color_component_value (app, 
										 (input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP), 
										 D_INCREASE);

			break;
		}
	}
}

static int auto_layout (int* remaining_free_space_on_y, int rect_height) {
	*remaining_free_space_on_y -= VERTICAL_PADDING;
	int result = *remaining_free_space_on_y;

	*remaining_free_space_on_y -= rect_height;
	return result;
}

static void draw_slider (int width, int height, lc_color color, float max_value, float value, int* remaining_y_space, bool is_selected) {
	// Calculate the slider rect
	lc_rect rect = { };
	rect.x = HORIZONTAL_PADDING;
	rect.y = auto_layout (remaining_y_space, height);
	rect.width = width;
	rect.height = height;

	// Draw the highlight if selected
	if (is_selected) {
		lc_color highlight_color = make_colorb (HIGHLIGHT_COLOR);
		lc_rect highlight_rect = { };
		highlight_rect.y = rect.y + VERTICAL_PADDING;	
		highlight_rect.width = width + (2 * HORIZONTAL_PADDING);
		highlight_rect.height = height + (2 * VERTICAL_PADDING);

		opengl_rect (highlight_rect, highlight_color);
	}

	// Draw slider
	opengl_rect (rect, color);

	byte value_in_bytes = (byte)((value * 255.0f) / max_value);
	byte max_value_in_bytes = (byte)(max_value * 255.0f);

	// Draw slider handle
	lc_color handle_color = make_colorb (HANDLE_COLOR);
	lc_rect handle_rect = { };
	handle_rect.width = SLIDER_HANDLE_WIDTH;
	handle_rect.height = height;
	handle_rect.x = rect.x + ((value_in_bytes * (width - handle_rect.width)) / max_value_in_bytes);
	handle_rect.y = rect.y;

	opengl_rect (handle_rect, handle_color);
}

void app_init (lc_memory* memory, platform_api platform, int client_width, int client_height) {
	lc_app* app = (lc_app*)memory -> storage;
	app -> current_color = make_colorb (DEFAULT_COLOR);
	app -> current_component = CC_R;

	app -> client_width = client_width;
	app -> client_height = client_height;

	app -> platform = platform;
}

void app_update (lc_memory* memory, lc_input input) {
	lc_app* app = (lc_app*)memory -> storage;

	handle_input (app, input);

	lc_color clear_color = make_colorb (CLEAR_COLOR);
	opengl_clear (app -> client_width, app -> client_height, clear_color);

	int remaining_y_space = app -> client_height;

	// (0,0) is bottom left, (width, height) is top right 
	lc_color color = app -> current_color;
	lc_rect color_rect = { };
	color_rect.width = app -> client_width - (2 * HORIZONTAL_PADDING);
	color_rect.height = app -> client_height / 3;
	color_rect.x = HORIZONTAL_PADDING;
	color_rect.y = auto_layout (&remaining_y_space, color_rect.height);
	opengl_rect (color_rect, color);

	int slider_width = app -> client_width - (2 * HORIZONTAL_PADDING);
	draw_slider (slider_width, SLIDER_HEIGHT, make_colorb (255, 0, 0), 1.0f, app -> current_color.r,
				 &remaining_y_space, app -> current_component == CC_R);
	draw_slider (slider_width, SLIDER_HEIGHT, make_colorb (0, 255, 0), 1.0f, app -> current_color.g,
				 &remaining_y_space, app -> current_component == CC_G);
	draw_slider (slider_width, SLIDER_HEIGHT, make_colorb (0, 0, 225), 1.0f, app -> current_color.b,
				 &remaining_y_space, app -> current_component == CC_B);
}
