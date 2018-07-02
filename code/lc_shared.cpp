#include "lc_shared.h"

lc_color make_colorf (float r, float g, float b) {
	return { r, g, b };
}

lc_color make_colorb (byte r, byte g, byte b) {
	return make_colorf ((float)r / 255, 
					    (float)g / 255,
					    (float)b / 255);
}
