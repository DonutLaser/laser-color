#if !defined (LC_GUI_LAYOUT_H)
#define LC_GUI_LAYOUT_H

#define HORIZONTAL_PADDING 10 
#define VERTICAL_PADDING 5 
#define SPACE 10

// Forward declarations
struct lc_rect;

struct layout_info {
	int client_width;
	int client_height;
	int remaining_width;
	int remaining_height;

	bool horizontal_group;
};

void layout_set_client_dimensions (layout_info* info, int width, int height);

void layout_begin_horizontal_group (layout_info* info);
void layout_end_horizontal_group (layout_info* info, int group_height);

void layout_auto_position (layout_info* info, lc_rect* rect);
void layout_space (layout_info* info, int amount);
void layout_space (layout_info* info);

#endif