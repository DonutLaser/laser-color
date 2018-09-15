#if !defined (LC_OPENGL) 
#define LC_OPENGL

// Forward declarations
struct lc_rect;
struct lc_image;
struct lc_font;
union lc_color;

void opengl_init (int width, int height);
void opengl_clear (int width, int height, lc_color color);
void opengl_rect (lc_rect rect, lc_color color);
void opengl_rect (lc_rect rect, lc_color color, lc_image image);
void opengl_text (int baseline_x, int baseline_y, lc_color color, lc_color shadow_color, lc_font font, char* text, bool shadow);

#endif