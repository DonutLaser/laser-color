#if !defined (LC_FONT_H)
#define LC_FONT_H

#include "lc_shared.h"

enum align_style { AS_LEFT, AS_CENTER, AS_RIGHT };

struct lc_font_character {
	lc_image bitmap;
	vector2 offset;
	int advance;
};

struct lc_font {
	lc_font_character chars[128];
	void* freetype_face;
	bool has_kerning;
};

bool font_load (const char* font_path, int pixel_size, lc_font* result);
int font_get_kerning (lc_font font, char left, char right);
int font_get_text_width (lc_font font, char* text);
void font_cleanup (lc_font font);

#endif