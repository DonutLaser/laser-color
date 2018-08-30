#include "lc_opengl.h"

// #ifdef WIN32
#include <windows.h>
// #endif
#include <gl/gl.h>
#include <stdio.h>

#include "lc_shared.h"

static GLuint texture_handles; // Only one for now

static void opengl_set_screenspace (int width, int height) {
    glMatrixMode (GL_TEXTURE);
    glLoadIdentity ();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    float a = 2.0f / width;
    float b = 2.0f / height;
    float projection[] = {
    	 a,  0,  0,  0,
    	 0,  b,  0,  0,
    	 0,  0,  1,  0,
    	-1, -1,  0,  1
    };
    glLoadMatrixf(projection);
}

void opengl_init (int width, int height) {
    glGenTextures (1, &texture_handles);

    opengl_set_screenspace (width, height);
}

void opengl_clear (int width, int height, lc_color color) {
	glViewport (0, 0, width, height);
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

void opengl_rect (lc_rect rect, lc_color color, lc_image image) {
    glBindTexture (GL_TEXTURE_2D, texture_handles);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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
}
