#if !defined (LC_SHARED)
#define LC_SHARED

#define byte unsigned char

struct lc_rect {
	int x;
	int y;
	int width;
	int height;
};

struct lc_image { 
	int width;
	int height;

	void* data;
};

struct lc_font_character {
	lc_image bitmap;
	int offset_x;
	int offset_y;
	int advance;
};

struct lc_font {
	lc_font_character chars[128];
	void* freetype_face;
	// FT_FaceRec freetype_face;
	bool has_kerning;
};

union lc_color {
	struct {
		float r;
		float g;
		float b;
	};

	float rgb[3];
};

lc_color make_colorf (float r, float g, float b);
lc_color make_colorb (byte r, byte g, byte b);

byte color_component_f2b (float value);

bool load_font (const char* font_path, int pixel_size, lc_font* result);
int get_kerning (lc_font font, char left, char right);

#endif