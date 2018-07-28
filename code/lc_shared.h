#if !defined (LC_SHARED)
#define LC_SHARED

#define byte unsigned char

struct lc_rect {
	int x;
	int y;
	int width;
	int height;
};

struct lc_color {
	float r;
	float g;
	float b;
};

lc_color make_colorf (float r, float g, float b);
lc_color make_colorb (byte r, byte g, byte b);

byte color_component_f2b (float value);

#endif