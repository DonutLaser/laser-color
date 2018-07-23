#include "lc_opengl.h"

// #ifdef WIN32
#include <windows.h>
// #endif
#include <gl/gl.h>

#include "lc_shared.h"

void opengl_set_screenspace (int width, int height) {
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