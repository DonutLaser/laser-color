#include "lc.h"

#include <stdio.h>
#include <stdlib.h>

#include "lc_platform.h"
#include "lc_shared.h"
#include "lc_memory.h"
#include "lc_opengl.h"
#include "lc_gui_layout.h"

#define clamp_value(x, min, max) (x < min) ? min : ((x > max) ? max : x)

#define HORIZONTAL_PADDING 10 
#define VERTICAL_PADDING 5 
#define SLIDER_HEIGHT 15 
#define SLIDER_HANDLE_WIDTH 8
#define SWATCH_HEIGHT 15
#define SWATCH_WIDTH 15
#define CLEAR_COLOR 200, 200, 200
#define DEFAULT_COLOR 255, 255, 255 
#define HIGHLIGHT_COLOR 160, 160, 160
#define HANDLE_COLOR 220, 220, 220

#define KEY_COMMA 	0xBC
#define KEY_POINT 	0xBE
#define KEY_J		0x4A
#define KEY_K		0x4B
#define KEY_N 		0x4E
#define KEY_ALPHA_0 0x30
#define KEY_ALPHA_4 0x34

#define SINGLE_STEP 0.00392f // 1/255
#define MEDIUM_STEP 0.0392f  // 10/255
#define FULL_STEP 1.0f

#define PATH_MAX 128 
#define BUFFER_SIZE 4096

enum change_direction { D_INCREASE = 1, D_DECREASE = -1 };

static byte string_to_byte (char* str) {
	byte result = 0;
	int multiplier = 100;

	for (int i = 0; i < 3; ++i) {
		if (str[i] == '\0')
			break;

		result += (str[i] - 0x30) * multiplier;
		multiplier /= 10;
	}

	return result;
}

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

static bool add_color_to_color_library (lc_color_library* swatches, lc_color color) {
	if (swatches -> count + 1 < MAX_COLORS_IN_LIBRARY) {
		swatches -> colors[swatches -> count++] = color;
		return true;
	}

	return false;
}

static void save_color_library (lc_app* app) {
	char buffer[BUFFER_SIZE];
	buffer[0] = '\0';
	
	int bytes_written = 0;

	app -> platform.log ("Found %d colors in the library. Saving... \n", app -> color_swatches.count);

	for (int i = 0; i < app -> color_swatches.count; ++i) {
		byte r = color_component_f2b (app -> color_swatches.colors[i].r);
		byte g = color_component_f2b (app -> color_swatches.colors[i].g);
		byte b = color_component_f2b (app -> color_swatches.colors[i].b);

		bytes_written = sprintf_s (buffer, "%s%d %d %d\n", buffer, r, g, b);
		buffer[bytes_written] = '\0';
	}

	if (app -> platform.write_file (app -> color_library_file.handle, buffer, bytes_written, WM_OVERWRITE))
		app -> platform.log ("Color library successfully saved at %s.\n", app -> color_library_file.path);
	else
		app -> platform.log ("Unexpected error when saving the color library at %s.\n", app -> color_library_file.path);
}

static void load_color_library (lc_app* app) {
	char* buffer;
	if (app -> platform.read_file (app -> color_library_file.handle, &buffer)) {
		char component[4];
		int current_char = 0;
		byte rgb[3];
		int current_component = 0;

		while (*buffer != '\0') {
			if (*buffer == ' ') {
				component[current_char] = '\0';
				app -> platform.log ("Parsed string %s\n", component);
				byte value = string_to_byte (component); 
				app -> platform.log ("Value: %d\n", value);
				rgb[current_component++] = value;

				current_char = 0;
			}
			else if (*buffer == '\n') {
				component[current_char] = '\0';
				app -> platform.log ("Parsed string %s\n", component);
				byte value = string_to_byte (component);
				app -> platform.log ("Value: %d\n", value);
				rgb[current_component] = value;

				add_color_to_color_library (&app -> color_swatches, 
											make_colorb (rgb[0], rgb[1], rgb[2]));

				current_char = 0;
				current_component = 0;
			}
			else
				component[current_char++] = *buffer;

			++buffer;
		}

		app -> platform.log ("Successfully added %d colors to the library\n", app -> color_swatches.count);
	}
	else
		app -> platform.log ("Could not read the color library file.\n");
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
		case KEY_N: {
			if (input.modifier & M_CTRL) {
				if (add_color_to_color_library (&app -> color_swatches, app -> current_color))
					app -> platform.log ("Successfully added new color to the library.\n");
				else
					app -> platform.log ("New color could not be added to the library. Library is full.\n");
			}

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

static void draw_slider (layout_info* layout, int width, int height, lc_color color, float max_value, float value, bool is_selected) {
	// Calculate the slider rect
	lc_rect rect = { };
	rect.width = width;
	rect.height = height;
	layout_auto_position (layout, &rect);

	// Draw the highlight if selected
	if (is_selected) {
		lc_color highlight_color = make_colorb (HIGHLIGHT_COLOR);

		// Find a way to position the highlight via layout methods 
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

static void draw_color_swatch (layout_info* layout, int width, int height, lc_color color) {
	lc_rect rect = { };
	rect.width = width;
	rect.height = height;
	layout_auto_position (layout, &rect);

	opengl_rect (rect, color);
}

void app_init (lc_memory* memory, platform_api platform, int client_width, int client_height) {
	lc_app* app = (lc_app*)memory -> storage;
	app -> platform = platform;

	app -> platform.log ("Initializing application...\n");

	app -> current_color = make_colorb (DEFAULT_COLOR);
	app -> current_component = CC_R;

	app -> client_width = client_width;
	app -> client_height = client_height;

	app -> color_swatches = { };

	app -> color_library_file.path = (char*)malloc (sizeof (char) * PATH_MAX);
	sprintf_s (app -> color_library_file.path, PATH_MAX, "%s", "D:/test_color_library.lclib");
	app -> color_library_file.handle = app -> platform.open_file ("D:/test_color_library.lclib");


	if (app -> color_library_file.handle) {
		app -> platform.log ("Color library file was successfully opened.\n");
		app -> platform.log ("Loading color library...\n");
		load_color_library (app);
		app -> platform.log ("Color library loaded.\n");
	}
	else
		app -> platform.log ("Error opening the color library file.\n");
}

void app_update (lc_memory* memory, lc_input input) {
	lc_app* app = (lc_app*)memory -> storage;

	handle_input (app, input);

	layout_info layout = { };
	layout_set_client_dimensions (&layout, app -> client_width, app -> client_height);

	lc_color clear_color = make_colorb (CLEAR_COLOR);
	opengl_clear (app -> client_width, app -> client_height, clear_color);

	// (0,0) is bottom left, (width, height) is top right 
	lc_color color = app -> current_color;
	lc_rect color_rect = { };
	color_rect.width = app -> client_width - (2 * HORIZONTAL_PADDING);
	color_rect.height = app -> client_height / 3;
	layout_auto_position (&layout, &color_rect);
	opengl_rect (color_rect, color);

	int slider_width = app -> client_width - (2 * HORIZONTAL_PADDING);
	draw_slider (&layout, slider_width, SLIDER_HEIGHT, make_colorb (255, 0, 0), 1.0f, app -> current_color.r,
				 app -> current_component == CC_R);
	draw_slider (&layout, slider_width, SLIDER_HEIGHT, make_colorb (0, 255, 0), 1.0f, app -> current_color.g,
				 app -> current_component == CC_G);
	draw_slider (&layout, slider_width, SLIDER_HEIGHT, make_colorb (0, 0, 225), 1.0f, app -> current_color.b,
				 app -> current_component == CC_B);

	layout_space (&layout);

	layout_begin_horizontal_group (&layout); {
		for (int i = 0; i < app -> color_swatches.count; ++i)
			draw_color_swatch (&layout, SWATCH_WIDTH, SWATCH_HEIGHT, app -> color_swatches.colors[i]);
	}
	layout_end_horizontal_group (&layout, SWATCH_HEIGHT);
}

void app_close (lc_memory* memory) {
	lc_app* app = (lc_app*)memory -> storage;
	
	save_color_library (app);

	free (app -> color_library_file.path);
}
