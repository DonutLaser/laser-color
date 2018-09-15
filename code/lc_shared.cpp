#include "lc_shared.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype/ftbitmap.h"

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

bool load_font (const char* font_path, int pixel_size, lc_font* result) {
	FT_Library freetype_lib;
	if (FT_Init_FreeType (&freetype_lib))
		return false;

	FT_Face font;
	if (FT_New_Face (freetype_lib, font_path, 0, &font))
		return false;

	FT_Set_Pixel_Sizes (font, 0, pixel_size);

	// Load 128 ASCII characters
	for (byte c = 0; c < 128; ++c) {
		if (FT_Load_Char (font, c, FT_LOAD_RENDER))
			continue;

		lc_font_character new_char = { };
		new_char.offset_x = font -> glyph -> bitmap_left;
		new_char.offset_y = font -> glyph -> bitmap_top;
		new_char.advance = font -> glyph -> advance.x;

		int w = font -> glyph -> bitmap.width;
		int h = font -> glyph -> bitmap.rows;
		new_char.bitmap = { };
		new_char.bitmap.width = w;
		new_char.bitmap.height = h;

		new_char.bitmap.data = malloc (w * h);
		for (int i = 0; i < new_char.bitmap.height; ++i) {
			for (int j = 0; j < new_char.bitmap.width; ++j)
				((byte*)new_char.bitmap.data)[(w * i) + j] = font -> glyph -> bitmap.buffer[(w * i) + j];
		}

		result -> chars[c] = new_char;
	}

	result -> freetype_face = &font;
	result -> has_kerning = FT_HAS_KERNING (font);

	// FT_Done_Face (font);
	// FT_Done_FreeType (freetype_lib);

	return true;
}

int get_kerning (lc_font font, char left, char right) {
	FT_Face face = *((FT_Face*)font.freetype_face);

	if (!font.has_kerning || !left || !right)
		return 0;

	int left_index = FT_Get_Char_Index (face, left);
	int right_index = FT_Get_Char_Index (face, right);
	FT_Vector kern;
	FT_Get_Kerning (face, left, right, FT_KERNING_DEFAULT, &kern);

	return kern.x >> 6;
}
