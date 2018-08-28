#if !defined (LC_OPENGL) 
#define LC_OPENGL

// Forward declarations
struct lc_rect;
struct lc_image;
union lc_color;

void opengl_init (int width, int height);
void opengl_clear (int width, int height, lc_color color);
void opengl_rect (lc_rect rect, lc_color color);
void opengl_rect (lc_rect rect, lc_color color, lc_image image);

#endif