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
#define SWATCH_AREA_HEIGHT 35
#define OUTLINE_WIDTH 1

#define CLEAR_COLOR 200, 200, 200
#define SWATCH_AREA_COLOR 170, 170, 170
#define OUTLINE_COLOR 80, 80, 80
#define DEFAULT_COLOR 255, 255, 255 
#define HANDLE_COLOR 220, 220, 220

#define KEY_ALPHA_0 0x30
#define KEY_B 		0x42
#define KEY_D		0x44
#define KEY_F		0x46
#define KEY_H 		0x48
#define KEY_I 		0x49
#define KEY_J		0x4A
#define KEY_L 		0x4C
#define KEY_K		0x4B
#define KEY_R 		0x52
#define KEY_U 		0x55
#define KEY_W		0x57
#define KEY_X 		0x58

#define SINGLE_STEP 0.00392f // 1/255
#define MEDIUM_STEP 0.0392f  // 10/255
#define FULL_STEP 1.0f

#define PATH_MAX 128 
#define BUFFER_SIZE 4096

enum change_direction { D_INCREASE = 1, D_DECREASE = -1 };
enum color_conversion_format { CF_BYTE, CF_FLOAT, CF_HEX };

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

static void copy_color_to_clipboard (lc_app* app, const char* format, lc_color color, color_conversion_format conversion) {
	int buffer_size;
	if (conversion == CF_BYTE)
		buffer_size = 14;
	else if (conversion == CF_FLOAT)
		buffer_size = 17;
	else
		buffer_size = 8;

	char* buffer = (char*)malloc (sizeof (char) * buffer_size);
	if (conversion == CF_BYTE || conversion == CF_HEX) {
		sprintf_s (buffer, buffer_size, format, 
			   	   (int)(color.r * 255),
			   	   (int)(color.g * 255),
			   	   (int)(color.b * 255));
	}
	else if (conversion == CF_FLOAT)
		sprintf_s (buffer, buffer_size, format, color.r, color.g, color.b);

	app -> platform.copy_to_clipboard (buffer);
	app -> platform.log ("Copied '%s' into the clipboard.", buffer);
}

static void change_color_component_value (float* component, float amount, change_direction direction) {
	*component = clamp_value (*component + ((int)direction * amount), 0.0f, 1.0f);
}

static void toggle_color_component (float* component) {
	change_color_component_value (component, FULL_STEP, 
								  (*component >= 0.5f) ? D_DECREASE :  D_INCREASE);
}

static void change_color_swatch (lc_app* app, change_direction direction, bool switch_to_newly_added_swatch = false) {
	if (app -> color_swatches.count == 0) {
		app -> current_swatch_index = -1;
		return;
	}

	if (switch_to_newly_added_swatch) {
		app -> current_swatch_index = app -> color_swatches.count - 1;
		return;
	}

	app -> current_swatch_index += (int)direction;
	if (app -> current_swatch_index == app -> color_swatches.count)
		app -> current_swatch_index = 0;
	else if (app -> current_swatch_index == -1)
		app -> current_swatch_index = app -> color_swatches.count - 1;

	app -> current_color = app -> color_swatches.colors[app -> current_swatch_index];
}

static void replace_selected_swatch (lc_app* app) {
	app -> color_swatches.colors[app -> current_swatch_index] = app -> current_color;
}

static void remove_selected_swatch (lc_app* app) {
	for (int i = app -> current_swatch_index; i < app -> color_swatches.count - 1; ++i)
		app -> color_swatches.colors[i] = app -> color_swatches.colors[i + 1];

	--app -> color_swatches.count;

	if (app -> current_swatch_index == app -> color_swatches.count)
		change_color_swatch (app, D_DECREASE);
}

static void make_selected_swatch_current_color (lc_app* app) {
	app -> current_color = app -> color_swatches.colors[app -> current_swatch_index];
}

static bool add_color_to_color_library (lc_app* app, lc_color color) {
	if (app -> color_swatches.count < MAX_COLORS_IN_LIBRARY) {
		app -> color_swatches.colors[app -> color_swatches.count++] = color;

		change_color_swatch (app, D_INCREASE, true);
		return true;
	}

	return false;
}

static void save_color_library (lc_app* app) {
	char buffer[BUFFER_SIZE];
	buffer[0] = '\0';
	
	int bytes_written = 0;

	app -> platform.log ("Found %d colors in the library", app -> color_swatches.count);
	if (app -> color_swatches.count == 0) {
		app -> platform.log (" ");
		return;
	}
	else
		app -> platform.log ("Saving...");

	for (int i = 0; i < app -> color_swatches.count; ++i) {
		byte r = color_component_f2b (app -> color_swatches.colors[i].r);
		byte g = color_component_f2b (app -> color_swatches.colors[i].g);
		byte b = color_component_f2b (app -> color_swatches.colors[i].b);

		bytes_written = sprintf_s (buffer, "%s%d %d %d\n", buffer, r, g, b);
		buffer[bytes_written] = '\0';
	}

	if (app -> platform.write_file (app -> color_library_file.handle, buffer, bytes_written, WM_OVERWRITE))
		app -> platform.log ("Color library successfully saved at %s.", app -> color_library_file.path);
	else
		app -> platform.log ("Unexpected error when saving the color library at %s.", app -> color_library_file.path);
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
				byte value = string_to_byte (component); 
				rgb[current_component++] = value;

				current_char = 0;
			}
			else if (*buffer == '\n') {
				component[current_char] = '\0';
				byte value = string_to_byte (component);
				rgb[current_component] = value;

				add_color_to_color_library (app, 
											make_colorb (rgb[0], rgb[1], rgb[2]));

				current_char = 0;
				current_component = 0;
			}
			else
				component[current_char++] = *buffer;

			++buffer;
		}

		if (app -> color_swatches.count != 0)
			app -> platform.log ("Successfully added %d colors to the library", app -> color_swatches.count);
	}
	else
		app -> platform.log ("Could not read the color library file.");
}

static void handle_input (lc_app* app, lc_input input) {
	switch (input.key) {
		case KEY_J: 
		case KEY_K: {
			int index = clamp_value (app -> current_component_index + (input.key == KEY_J ? 1 : -1), 0, 2);
			app -> current_component = &app -> current_color.rgb[index];
			app -> current_component_index = index;
			break;
		}
		case KEY_I: {
			if (add_color_to_color_library (app, app -> current_color))
				app -> platform.log ("Successfully added new color to the library.");
			else
				app -> platform.log ("New color could not be added to the library. Library is full.");

			break;
		}
		case KEY_ALPHA_0: {
			// Switch it from 0 to 255 and vice versa
			toggle_color_component (app -> current_component);	
			break;
		}
		case KEY_H:
		case KEY_L: {
			float amount = input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP;
			change_color_component_value (app -> current_component, amount,
										  input.key == KEY_H ? D_DECREASE : D_INCREASE);

			break;
		}
		case KEY_W:
		case KEY_B: {
			change_color_swatch (app, input.key == KEY_W ? D_INCREASE : D_DECREASE);
			break;
		}
		case KEY_R: {
			replace_selected_swatch (app);
			break;
		}
		case KEY_X: {
			if (input.modifier & M_CTRL)
				copy_color_to_clipboard (app, "#%.2x%.2x%.2x\0", app -> current_color, CF_HEX);
			else
				remove_selected_swatch (app);

			break;
		}
		case KEY_U: {
			make_selected_swatch_current_color (app);
			break;
		}
		case KEY_D: {
			if (!(input.modifier & M_CTRL)) 
				return;

			copy_color_to_clipboard (app, "%d, %d, %d\0", app -> current_color, CF_BYTE);
			break;
		}
		case KEY_F: {
			if (!(input.modifier & M_CTRL)) 
				return;

			copy_color_to_clipboard (app, "%.1ff, %.1ff, %.1ff\0", app -> current_color, CF_FLOAT);
			break;
		}
	}
}

static void draw_outline (lc_rect rect) {
	lc_color color = make_colorb (OUTLINE_COLOR);
	lc_rect outline_rect = { };
	outline_rect.width = rect.width + (OUTLINE_WIDTH* 2);
	outline_rect.height = rect.height + (OUTLINE_WIDTH * 2);
	outline_rect.x = rect.x - OUTLINE_WIDTH;
	outline_rect.y = rect.y + OUTLINE_WIDTH;

	opengl_rect (outline_rect, color);
}

static void draw_slider (layout_info* layout, int width, int height, lc_color color, float max_value, float value, bool is_selected) {
	// Calculate the slider rect
	lc_rect rect = { };
	rect.width = width;
	rect.height = height;
	layout_auto_position (layout, &rect);

	// Draw the outline if selected
	if (is_selected)
		draw_outline (rect);

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

static void draw_color_swatch (layout_info* layout, int width, int height, lc_color color, bool is_selected) {
	lc_rect rect = { };
	rect.width = width;
	rect.height = height;
	layout_auto_position (layout, &rect);

	draw_outline (rect);

	opengl_rect (rect, color);

	if (is_selected) {
		lc_color highlight_color = make_colorb (255, 255, 255);
		lc_rect highlight_rect = { };
		highlight_rect.width = rect.width;
		highlight_rect.height = rect.height / 4;
		highlight_rect.x = rect.x;
		highlight_rect.y = highlight_rect.height;

		opengl_rect (highlight_rect, highlight_color);
	}
}

void app_init (lc_memory* memory, platform_api platform, int client_width, int client_height) {
	lc_app* app = (lc_app*)memory -> storage;
	app -> platform = platform;

	app -> platform.log ("Initializing application...");
	app -> current_color = make_colorb (DEFAULT_COLOR);
	app -> current_component = &app -> current_color.r;
	app -> current_component_index = 0;

	app -> client_width = client_width;
	app -> client_height = client_height;

	app -> color_swatches = { };

	app -> color_library_file.path = (char*)malloc (sizeof (char) * PATH_MAX);
	sprintf_s (app -> color_library_file.path, PATH_MAX, "%s", "D:/test_color_library.lclib");
	app -> color_library_file.handle = app -> platform.open_file ("D:/test_color_library.lclib");

	if (app -> color_library_file.handle) {
		app -> platform.log ("Color library file was successfully opened.");
		app -> platform.log ("Loading color library...");
		load_color_library (app);
		app -> platform.log ("Color library loaded.");
	}
	else
		app -> platform.log ("Error opening the color library file.");

	app -> current_swatch_index = -1;
}

void app_update (lc_memory* memory, lc_input input) {
	lc_app* app = (lc_app*)memory -> storage;

	handle_input (app, input);

	layout_info layout = { };
	layout_set_client_dimensions (&layout, app -> client_width, app -> client_height);

	lc_color clear_color = make_colorb (CLEAR_COLOR);
	opengl_clear (app -> client_width, app -> client_height, clear_color);

	lc_color swatch_area_color = make_colorb (SWATCH_AREA_COLOR);
	lc_rect swatch_area_rect = { };
	swatch_area_rect.width = app -> client_width;
	swatch_area_rect.height = SWATCH_AREA_HEIGHT;
	swatch_area_rect.x = 0;
	swatch_area_rect.y = SWATCH_AREA_HEIGHT;
	opengl_rect (swatch_area_rect, swatch_area_color);

	// (0,0) is bottom left, (width, height) is top right 
	lc_color color = app -> current_color;
	lc_rect color_rect = { };
	color_rect.width = app -> client_width - (2 * HORIZONTAL_PADDING);
	color_rect.height = app -> client_height / 3;
	layout_auto_position (&layout, &color_rect);
	opengl_rect (color_rect, color);

	layout_space (&layout);

	int slider_width = app -> client_width - (2 * HORIZONTAL_PADDING);
	draw_slider (&layout, slider_width, SLIDER_HEIGHT, make_colorb (255, 0, 0), 1.0f, app -> current_color.r,
				 app -> current_component == &app -> current_color.r);
	draw_slider (&layout, slider_width, SLIDER_HEIGHT, make_colorb (0, 255, 0), 1.0f, app -> current_color.g,
				 app -> current_component == &app -> current_color.g);
	draw_slider (&layout, slider_width, SLIDER_HEIGHT, make_colorb (0, 0, 225), 1.0f, app -> current_color.b,
				 app -> current_component == &app -> current_color.b);

	layout_space (&layout, 23);

	layout_begin_horizontal_group (&layout); {
		for (int i = 0; i < app -> color_swatches.count; ++i) {
			draw_color_swatch (&layout, SWATCH_WIDTH, 
							   SWATCH_HEIGHT, 
							   app -> color_swatches.colors[i], 
							   app -> current_swatch_index == i);
		}
	}
	layout_end_horizontal_group (&layout, SWATCH_HEIGHT);
}

void app_close (lc_memory* memory) {
	lc_app* app = (lc_app*)memory -> storage;
	
	save_color_library (app);

	free (app -> color_library_file.path);
}
