#include "lc_shared.h"

lc_color make_colorf (float r, float g, float b) {
	lc_color result = { };
	result.r = r;
	result.g = g;
	result.b = b;

	return result;
}

lc_color make_colorb (byte r, byte g, byte b) {
	return make_colorf ((float)r / 255, 
					    (float)g / 255,
					    (float)b / 255);
}

byte color_component_f2b (float value) {
	return (byte)(value * 255);
}