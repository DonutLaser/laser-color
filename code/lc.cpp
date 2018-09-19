#include "lc.h"

#include <stdio.h>
#include <stdlib.h>

#include "lc_platform.h"
#include "lc_memory.h"
#include "lc_opengl.h"
#include "lc_gui_layout.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define STBI_MSC_SECURE_CRT
// #include "../third_party/stb_image_write.h"

#define clamp_value(x, min, max) (x < min) ? min : ((x > max) ? max : x)

#define MAJOR_MARGIN 15
#define MINOR_MARGIN 8

#define TITLE_BAR_HEIGHT 32
#define STATUS_BAR_HEIGHT 15
#define COLOR_RECT_HEIGHT 120
#define SLIDER_HEIGHT 23 
#define SLIDER_HANDLE_WIDTH 8
#define SWATCH_WIDTH 31 
#define SWATCH_HEIGHT 24 
#define SWATCH_BAR_HEIGHT 54 
#define OUTLINE_WIDTH 2
#define MAJOR_FONT_SIZE 15 

#define CLEAR_COLOR 175, 175, 175 
#define TITLE_BAR_COLOR 118, 118, 118
#define SWATCH_BAR_COLOR 81, 80, 80
#define STATUS_BAR_COLOR 58, 58, 58
#define SLIDER_BASE_COLOR 127, 127, 127
#define RED_SLIDER_COLOR 124, 53, 53
#define GREEN_SLIDER_COLOR 37, 107, 37
#define BLUE_SLIDER_COLOR 57, 57, 126 
#define OUTLINE_COLOR 206, 206, 209 
#define DEFAULT_COLOR 255, 255, 255 
#define HANDLE_COLOR 220, 220, 220
#define DIRTY_COLOR 255, 70, 70
#define SHADOW_COLOR 30, 30, 30
#define SLIDER_TEXT_COLOR 221, 221, 221

#define CLOSE_COLOR 58, 58, 58
#define CLOSE_HIGHLIGHT_COLOR 185, 60, 60
#define CLOSE_PRESSED_COLOR 214, 70, 70

#define MINIMIZE_COLOR 89, 88, 88
#define MINIMIZE_HIGHLIGHT_COLOR 135, 134, 134
#define MINIMIZE_PRESSED_COLOR 158, 157, 157

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
#define KEY_S 		0x53
#define KEY_U 		0x55
#define KEY_W		0x57
#define KEY_X 		0x58

#define SINGLE_STEP 0.00392f // 1/255
#define MEDIUM_STEP 0.0392f  // 10/255
#define FULL_STEP 1.0f

#define BUFFER_SIZE 4096

#define SLIDER_ARROW_LEFT_PATH "W:\\color\\data\\images\\slider_arrow_left.png"
#define SLIDER_ARROW_RIGHT_PATH "W:\\color\\data\\images\\slider_arrow_right.png"
#define SWATCH_ARROW_TOP "W:\\color\\data\\images\\swatch_arrow_top.png"
#define SWATCH_ARROW_BOTTOM "W:\\color\\data\\images\\swatch_arrow_bottom.png"
#define CLOSE_ICON "W:\\color\\data\\images\\close_icon.png"
#define MINIMIZE_ICON "W:\\color\\data\\images\\minimize_icon.png"

#define MAIN_FONT "W:\\color\\data\\fonts\\consola.ttf"

#define TITLE "Laser Color"

enum change_direction { D_INCREASE = 1, D_DECREASE = -1 };
enum color_conversion_format { CF_BYTE, CF_FLOAT, CF_HEX };

static bool is_point_in_rect (lc_rect rect, vector2 point) {
	return point.x >= rect.x && point.x <= rect.x + rect.width &&
		point.y <= rect.y && point.y >= rect.y - rect.height;
}

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
		app -> previous_color = app -> current_color;
		return;
	}

	app -> current_swatch_index += (int)direction;
	if (app -> current_swatch_index == app -> color_swatches.count)
		app -> current_swatch_index = 0;
	else if (app -> current_swatch_index < 0)
		app -> current_swatch_index = app -> color_swatches.count - 1;

	app -> current_color = app -> color_swatches.colors[app -> current_swatch_index];
	app -> previous_color = app -> current_color;
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

	app -> platform.log (false, "Found %d colors in the library", app -> color_swatches.count);
	if (app -> color_swatches.count == 0) {
		app -> platform.log (true, "Nothing to save.");
		return;
	}
	else
		app -> platform.log (false, "Saving...");

	for (int i = 0; i < app -> color_swatches.count; ++i) {
		byte r = color_component_f2b (app -> color_swatches.colors[i].r);
		byte g = color_component_f2b (app -> color_swatches.colors[i].g);
		byte b = color_component_f2b (app -> color_swatches.colors[i].b);

		bytes_written = sprintf_s (buffer, "%s%d %d %d\n", buffer, r, g, b);
		buffer[bytes_written] = '\0';
	}

	if (app -> platform.write_file (app -> color_library_file.handle, buffer, bytes_written, WM_OVERWRITE))
		app -> platform.log (true, " Success. Saved at %s.", app -> color_library_file.path);
	else
		app -> platform.log ("Could not write into %s.", app -> color_library_file.path);
}

static void load_color_library (lc_app* app) {
	char* buffer;
	if (app -> platform.read_file (app -> color_library_file.handle, &buffer, NULL)) {
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
			app -> platform.log (true, "Successfully added %d colors to the library", app -> color_swatches.count);
	}
	else
		app -> platform.log (true, "Could not read the color library file.");
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
				app -> color_library_is_dirty = true;
			else
				app -> platform.log (true, "New color could not be added to the library. Library is full.");

			break;
		}
		case KEY_ALPHA_0: {
			// Switch it from 0 to 255 and vice versa
			toggle_color_component (app -> current_component);	
			break;
		}
		case KEY_W:
		case KEY_B: {
			float amount = input.modifier & M_SHIFT ? MEDIUM_STEP : SINGLE_STEP;
			change_color_component_value (app -> current_component, amount,
										  input.key == KEY_B ? D_DECREASE : D_INCREASE);

			break;
		}
		case KEY_H:
		case KEY_L: {
			change_color_swatch (app, input.key == KEY_L ? D_INCREASE : D_DECREASE);
			break;
		}
		case KEY_R: {
			replace_selected_swatch (app);
			app -> color_library_is_dirty = true;
			break;
		}
		case KEY_X: {
			if (input.modifier & M_CTRL)
				copy_color_to_clipboard (app, "#%.2x%.2x%.2x\0", app -> current_color, CF_HEX);
			else {
				remove_selected_swatch (app);
				app -> color_library_is_dirty = true;
			}

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
		case KEY_S: { 
			if (input.modifier & M_CTRL) {
				if (app -> color_library_is_dirty) {
					save_color_library (app);
					app -> color_library_is_dirty = false;
				}
			}

			break;
		}
	}
}

static void draw_outline (lc_rect rect) {
	lc_color color = make_colorb (OUTLINE_COLOR);
	lc_rect outline_rect = { };
	outline_rect.width = rect.width + (OUTLINE_WIDTH * 2);
	outline_rect.height = rect.height + (OUTLINE_WIDTH * 2);
	outline_rect.x = rect.x - OUTLINE_WIDTH;
	outline_rect.y = rect.y + OUTLINE_WIDTH;

	opengl_rect (outline_rect, color);
}

static void draw_slider (lc_app* app, int y_position, lc_color main_color, float max, float value, bool is_selected) {
	lc_color base_color = make_colorb (SLIDER_BASE_COLOR);
	lc_rect rect = { };
	rect.width = app -> client_size.x - (2 * MAJOR_MARGIN);
	rect.height = SLIDER_HEIGHT;
	rect.x = MAJOR_MARGIN;
	rect.y = y_position;

	if (is_selected) {
		lc_image left_arrow = app -> ui_images[UI_SLIDER_ARROW_LEFT];
		lc_image right_arrow = app -> ui_images[UI_SLIDER_ARROW_RIGHT];

		lc_color arrow_color = make_colorb (DEFAULT_COLOR);
		lc_rect arrow_rect_left = { };
		arrow_rect_left.width = left_arrow.size.x;
		arrow_rect_left.height = left_arrow.size.y;
		arrow_rect_left.x = 0;
		arrow_rect_left.y = rect.y;
		opengl_image (arrow_rect_left, arrow_color, left_arrow);

		lc_rect arrow_rect_right = { };
		arrow_rect_right.width = right_arrow.size.x;
		arrow_rect_right.height = right_arrow.size.y;
		arrow_rect_right.x = MAJOR_MARGIN + rect.width;
		arrow_rect_right.y = rect.y;
		opengl_image (arrow_rect_right, arrow_color, right_arrow);

		draw_outline (rect);
	}

	opengl_rect (rect, base_color);

	lc_rect colored_rect = { };
	colored_rect.width = (int)((value * (float)rect.width) / max);
	colored_rect.height = rect.height;
	colored_rect.x = rect.x;
	colored_rect.y = rect.y;
	opengl_rect (colored_rect, main_color);

	char value_text[4];
	sprintf_s (value_text, "%d\0", (int)value);
	lc_color text_color = make_colorb (SLIDER_TEXT_COLOR);
	lc_color shadow_color = make_colorb (SHADOW_COLOR);
	lc_rect text_rect = rect;
	text_rect.y -= 16;
	opengl_text (text_rect, text_color, shadow_color, app -> main_font, value_text, AS_CENTER);
}

static void draw_color_swatch (lc_app* app, vector2 position, lc_color color, bool is_selected) {
	lc_rect rect = { };
	rect.width = SWATCH_WIDTH;
	rect.height = SWATCH_HEIGHT;
	rect.x = position.x;
	rect.y = position.y;

	if (is_selected) {
		lc_image top_arrow = app -> ui_images[UI_SWATCH_ARROW_TOP];
		lc_image bottom_arrow = app -> ui_images[UI_SWATCH_ARROW_BOTTOM];

		lc_color arrow_color = make_colorb (DEFAULT_COLOR);
		lc_rect arrow_rect_top = { };
		arrow_rect_top.width = top_arrow.size.x;
		arrow_rect_top.height = top_arrow.size.y;
		arrow_rect_top.x = rect.x;
		arrow_rect_top.y = position.y + MAJOR_MARGIN;
		opengl_image (arrow_rect_top, arrow_color, top_arrow);

		lc_rect arrow_rect_bottom = { };
		arrow_rect_bottom.width = bottom_arrow.size.x;
		arrow_rect_bottom.height = bottom_arrow.size.y;
		arrow_rect_bottom.x = rect.x;
		arrow_rect_bottom.y = position.y- rect.height;
		opengl_image (arrow_rect_bottom, arrow_color, bottom_arrow);

		draw_outline (rect);
	}

	opengl_rect (rect, color);
}

static bool draw_button (lc_app* app, lc_rect rect, lc_color color, lc_color highlight_color, lc_color pressed_color, lc_image icon, lc_input input) {
	bool result = false;

	lc_color actual_color;
	if (is_point_in_rect (rect, input.mouse.position)) {
		actual_color = input.mouse.lmb_down ? pressed_color : highlight_color;

		if (input.mouse.lmb_up)
			result = true;
	}
	else
		actual_color = color;

	opengl_rect (rect, actual_color);
	opengl_image (rect, make_colorb (DEFAULT_COLOR), icon);

	return result;
}

void app_init (lc_memory* memory, platform_api platform, vector2 client_size, char* documents, vector2* title_bar_size) {
	lc_app* app = (lc_app*)memory -> storage;
	app -> platform = platform;

	app -> platform.log (true, "Initializing application...");
	app -> current_color = make_colorb (DEFAULT_COLOR);
	app -> previous_color = app -> current_color;
	app -> current_component = &app -> current_color.r;
	app -> current_component_index = 0;

	app -> client_size = client_size;
	*title_bar_size = { client_size.x - (2 * TITLE_BAR_HEIGHT), TITLE_BAR_HEIGHT };

	app -> color_swatches = { };

	app -> color_library_file.path = (char*)malloc (sizeof (char) * PATH_MAX);
	sprintf_s (app -> color_library_file.path, PATH_MAX, "%s/%s", documents, "test_color_library.lclib");
	app -> color_library_file.handle = app -> platform.open_file (app -> color_library_file.path);

	if (app -> color_library_file.handle) {
		app -> platform.log (true, "Color library file was successfully opened.");
		app -> platform.log (false, "Loading color library...");
		load_color_library (app);
		app -> platform.log (true, " Success.");
	}
	else
		app -> platform.log (true, "Could not open the color library file.");

	app -> current_swatch_index = -1;

	// Load images used for UI
	char image_paths[UI_COUNT][PATH_MAX] = { SLIDER_ARROW_LEFT_PATH, SLIDER_ARROW_RIGHT_PATH, SWATCH_ARROW_TOP, SWATCH_ARROW_BOTTOM, CLOSE_ICON, MINIMIZE_ICON };
	for (int i = 0; i < UI_COUNT; ++i) {
		int n;
		app -> platform.log (false, "Loading %s...", image_paths[i]);
		app -> ui_images[i].data = (void*)stbi_load (image_paths[i],
													 &app -> ui_images[i].size.x,
													 &app -> ui_images[i].size.y,
													 &n, 0);

		if (app -> ui_images[i].data)
			app -> platform.log (true, " Sucesss!");
		else
			app -> platform.log (true, " Unable to load the image!");
	}
	
	// Load font
	app -> platform.log (false, "Loading the %s font...", MAIN_FONT);
	if (!font_load (MAIN_FONT, MAJOR_FONT_SIZE, &app -> main_font))
		app -> platform.log (true, " Unable to load the font.");
	else
		app -> platform.log (true, " Success!");
}

void app_update (lc_memory* memory, lc_input input) {
	lc_app* app = (lc_app*)memory -> storage;

	// Convert the coordinate from 0,0 at the top to 0,0 at the bottom
	input.mouse.position.y = app -> client_size.y - input.mouse.position.y;

	handle_input (app, input);

	lc_color white_color = make_colorb (DEFAULT_COLOR);

	lc_color clear_color = make_colorb (CLEAR_COLOR);
	opengl_clear (app -> client_size, clear_color);

	// TITLE BAR
	lc_color title_bar_color = make_colorb (TITLE_BAR_COLOR);
	lc_rect title_bar_rect = { };
	title_bar_rect.width = app -> client_size.x;
	title_bar_rect.height = TITLE_BAR_HEIGHT;
	title_bar_rect.x = 0;
	title_bar_rect.y = app -> client_size.y;
	opengl_rect (title_bar_rect, title_bar_color);

	char title[] = { TITLE };
	lc_color shadow_color = make_colorb (SHADOW_COLOR);
	lc_rect title_rect = { 10, app -> client_size.y - 21,
						   title_bar_rect.width, title_bar_rect.height};
	opengl_text (title_rect, white_color, shadow_color, app -> main_font, title, AS_LEFT);

	lc_rect close_button_rect = { };
	close_button_rect.x = app -> client_size.x - TITLE_BAR_HEIGHT;
	close_button_rect.y = app -> client_size.y;
	close_button_rect.width = TITLE_BAR_HEIGHT;
	close_button_rect.height = TITLE_BAR_HEIGHT;
	if (draw_button (app, close_button_rect, make_colorb (CLOSE_COLOR), make_colorb (CLOSE_HIGHLIGHT_COLOR), 
					 make_colorb (CLOSE_PRESSED_COLOR), app -> ui_images[UI_CLOSE], input)) {
		app -> platform.close_application ();
	}

	lc_rect minimize_button_rect = close_button_rect;
	minimize_button_rect.x -= TITLE_BAR_HEIGHT;
	if (draw_button (app, minimize_button_rect, make_colorb (MINIMIZE_COLOR), make_colorb (MINIMIZE_HIGHLIGHT_COLOR), 
					 make_colorb (MINIMIZE_PRESSED_COLOR), app -> ui_images[UI_MINIMIZE], input)) {
		app -> platform.minimize_application ();
	} 

	// COLOR DISPLAY
	lc_rect main_color_rect = { };
	main_color_rect.width = (app -> client_size.x - (2 * MAJOR_MARGIN)) / 2;
	main_color_rect.height = COLOR_RECT_HEIGHT;
	main_color_rect.x = MAJOR_MARGIN;
	main_color_rect.y = app -> client_size.y - TITLE_BAR_HEIGHT - MAJOR_MARGIN;
	opengl_rect (main_color_rect, app -> current_color);

	lc_color prev_color = app -> previous_color;
	lc_rect prev_color_rect = { };
	prev_color_rect.width = main_color_rect.width; 
	prev_color_rect.height = main_color_rect.height;
	prev_color_rect.x = main_color_rect.width + MAJOR_MARGIN;
	prev_color_rect.y = main_color_rect.y;
	opengl_rect (prev_color_rect, prev_color);

	// SLIDERS
	draw_slider (app, main_color_rect.y - main_color_rect.height - MAJOR_MARGIN,
				 make_colorb (RED_SLIDER_COLOR), 255, color_component_f2b (app -> current_color.r),
				 app -> current_component == &app -> current_color.r);
	draw_slider (app, main_color_rect.y - main_color_rect.height - MAJOR_MARGIN - SLIDER_HEIGHT - MINOR_MARGIN,
				 make_colorb (GREEN_SLIDER_COLOR), 255, color_component_f2b (app -> current_color.g),
				 app -> current_component == &app -> current_color.g);
	draw_slider (app, main_color_rect.y - main_color_rect.height - MAJOR_MARGIN - SLIDER_HEIGHT - MINOR_MARGIN - SLIDER_HEIGHT - MINOR_MARGIN,
				 make_colorb (BLUE_SLIDER_COLOR), 255, color_component_f2b (app -> current_color.b),
				 app -> current_component == &app -> current_color.b);

	// SWATCHES
	lc_color swatch_bar_color = make_colorb (SWATCH_BAR_COLOR);
	lc_rect swatch_bar_rect = { };
	swatch_bar_rect.width = app -> client_size.x;
	swatch_bar_rect.height = SWATCH_BAR_HEIGHT;
	swatch_bar_rect.x = 0;
	swatch_bar_rect.y = SWATCH_BAR_HEIGHT + STATUS_BAR_HEIGHT; 
	opengl_rect (swatch_bar_rect, swatch_bar_color);

	for (int i = 0; i < app -> color_swatches.count; ++i) {
		vector2 position = { MAJOR_MARGIN + ((SWATCH_WIDTH + MINOR_MARGIN) * i),
										  swatch_bar_rect.y - MAJOR_MARGIN };
		draw_color_swatch (app, position, app -> color_swatches.colors[i], app -> current_swatch_index == i);
	}

	// STATUS BAR
	lc_color status_bar_color = make_colorb (STATUS_BAR_COLOR);
	lc_rect status_bar_rect = { };
	status_bar_rect.width = app -> client_size.x;
	status_bar_rect.height = STATUS_BAR_HEIGHT;
	status_bar_rect.x = 0;
	status_bar_rect.y = STATUS_BAR_HEIGHT;
	opengl_rect (status_bar_rect, status_bar_color);

	// if (app -> color_library_is_dirty) {
	// 	lc_color dirty_color = make_colorb (DIRTY_COLOR);
	// 	lc_rect dirty_rect = { };
	// 	dirty_rect.width = SWATCH_WIDTH / 3;
	// 	dirty_rect.height = SWATCH_HEIGHT + (OUTLINE_WIDTH * 2);
	// 	dirty_rect.x = 0;
	// 	dirty_rect.y = SWATCH_AREA_HEIGHT - ((SWATCH_AREA_HEIGHT - SWATCH_HEIGHT) / 2) + OUTLINE_WIDTH;

	// 	opengl_rect (dirty_rect, dirty_color);
	// }
}

void app_close (lc_memory* memory) {
	lc_app* app = (lc_app*)memory -> storage;

	free (app -> color_library_file.path);
	app -> platform.close_file (app -> color_library_file.handle);

	font_cleanup (app -> main_font);
}
