#if !defined (LC_SHARED)
#define LC_SHARED

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

#endif