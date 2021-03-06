#include "lc_opengl.h"

// #ifdef WIN32
#include <windows.h>
// #endif
#include <gl/gl.h>
#include <stdio.h>

#include "lc_shared.h"
#include "lc_font.h"

static GLuint texture_handles; // Only one for now

static void opengl_set_screenspace (vector2 size) {
    glMatrixMode (GL_TEXTURE);
    glLoadIdentity ();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    float a = 2.0f / size.x;
    float b = 2.0f / size.y;
    float projection[] = {
    	 a,  0,  0,  0,
    	 0,  b,  0,  0,
    	 0,  0,  1,  0,
    	-1, -1,  0,  1
    };
    glLoadMatrixf(projection);
}

static void draw_texture (lc_rect rect, lc_color color) {
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);

    glColor4f(color.r, color.g, color.b, 1.0f);

    glTexCoord2f (0.0f, 0.0f);
    glVertex2f ((float)rect.x, (float)rect.y);
    glTexCoord2f (1.0f, 0.0f);
    glVertex2f ((float)rect.x + (float)rect.width, (float)rect.y);
    glTexCoord2f (1.0f, 1.0f);
    glVertex2f ((float)rect.x + (float)rect.width, (float)rect.y - (float)rect.height);

    glTexCoord2f (0.0f, 0.0f);
    glVertex2f ((float)rect.x, (float)rect.y);
    glTexCoord2f (1.0f, 1.0f);
    glVertex2f ((float)rect.x + (float)rect.width, (float)rect.y - (float)rect.height);
    glTexCoord2f (0.0f, 1.0f);
    glVertex2f ((float)rect.x, (float)rect.y - (float)rect.height);

    glEnd();

    glDisable (GL_BLEND);   

    glDisable (GL_TEXTURE_2D);
}

static void draw_text (lc_rect rect, lc_color color, lc_font font, char* text, align_style alignment) {
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    int x_start = 0;

    if (alignment == AS_LEFT)
        x_start = rect.x;
    else {
        char* text_copy = text;
        int width = font_get_text_width (font, text_copy);

        x_start = alignment == AS_CENTER ? (rect.x + (rect.width / 2)) - (width / 2) :
            (rect.x + rect.width) - width;
    }

    char previous = 0;
    while (*text != '\0') {
        lc_font_character c = font.chars[*text];

        x_start += font_get_kerning (font, previous, *text);

        lc_rect char_rect = { };
        char_rect.x = x_start;
        char_rect.y = rect.y + c.offset.y;
        char_rect.width = c.bitmap.size.x;
        char_rect.height = c.bitmap.size.y;

        glBindTexture (GL_TEXTURE_2D, texture_handles);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_ALPHA, c.bitmap.size.x, c.bitmap.size.y, 0, GL_ALPHA, GL_UNSIGNED_BYTE, c.bitmap.data);

        draw_texture (char_rect, color);

        glBindTexture (GL_TEXTURE_2D, 0);

        int advance = char_rect.width == 0 ? c.advance >> 6 : char_rect.width;
        x_start += advance;

        previous = *text;
        ++text;
    }
}

void opengl_init (vector2 size) {
    glGenTextures (1, &texture_handles);

    opengl_set_screenspace (size);
}

void opengl_clear (vector2 size, lc_color color) {
	glViewport (0, 0, size.x, size.y);
	glClearColor (color.r, color.g, color.b, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);
}

void opengl_rect (lc_rect rect, lc_color color) {
    glBegin(GL_TRIANGLES);

    glColor4f(color.r, color.g, color.b, 0.0f);

    glVertex2f ((float)rect.x, (float)rect.y);
    glVertex2f ((float)rect.x + (float)rect.width, (float)rect.y);
    glVertex2f ((float)rect.x + (float)rect.width, (float)rect.y - (float)rect.height);

    glVertex2f ((float)rect.x, (float)rect.y);
    glVertex2f ((float)rect.x + (float)rect.width, (float)rect.y - (float)rect.height);
    glVertex2f ((float)rect.x, (float)rect.y - (float)rect.height);

    glEnd();
}

void opengl_image (lc_rect rect, lc_color color, lc_image image) {
    glBindTexture (GL_TEXTURE_2D, texture_handles);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, image.size.x, image.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

    draw_texture (rect, color);

    glBindTexture (GL_TEXTURE_2D, 0);
}

void opengl_text (lc_rect rect, lc_color color, lc_color shadow_color, lc_font font, char* text, align_style alignment) {
    lc_rect shadow_rect = rect;
    --shadow_rect.y;

    draw_text (shadow_rect, shadow_color, font, text, alignment);
    draw_text (rect, color, font, text, alignment);
}
