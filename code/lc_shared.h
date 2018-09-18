#if !defined (LC_SHARED)
#define LC_SHARED

#define byte unsigned char

union vector2 {
	struct {
		int x;
		int y;
	};

	int values[2];
};

struct lc_rect {
	int x;
	int y;
	int width;
	int height;
};

struct lc_image { 
	vector2 size;

	void* data;
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

#endif