#include "lc_gui_layout.h"

#include "lc_shared.h"

void layout_set_client_dimensions (layout_info* info, int width, int height) {
	info -> client_width = info -> remaining_width = width;
	info -> client_height = info -> remaining_height = height;
}

void layout_begin_horizontal_group (layout_info* info) {
	info -> horizontal_group = true;
	info -> remaining_height -= VERTICAL_PADDING;
}

void layout_end_horizontal_group (layout_info* info, int group_height) {
	info -> horizontal_group = false;
	info -> remaining_height -= group_height;
	info -> remaining_width = info -> client_width;
}

void layout_auto_position (layout_info* info, lc_rect* rect) {
	if (!info -> horizontal_group) {
		info -> remaining_height -= VERTICAL_PADDING;
		rect -> x = HORIZONTAL_PADDING;
		rect -> y = info -> remaining_height;
		info -> remaining_height -= rect -> height;
	}
	else {
		info -> remaining_width -= HORIZONTAL_PADDING;
		rect -> x = info -> client_width - info -> remaining_width;
		rect -> y = info -> remaining_height;
		info -> remaining_width -= rect -> width;
	}
}