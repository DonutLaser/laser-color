#if !defined (LC_OPENGL) 
#define LC_OPENGL

// Forward declarations
struct lc_rect;
struct lc_color;

void opengl_set_screenspace (int width, int heigth);
void opengl_clear (int width, int height, lc_color color);
void opengl_rect (lc_rect rect, lc_color color);

#endif