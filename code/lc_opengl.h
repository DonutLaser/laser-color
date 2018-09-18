#if !defined (LC_OPENGL) 
#define LC_OPENGL

// Forward declarations
struct lc_rect;
struct lc_image;
union lc_color;
union vector2;
struct lc_font;
enum align_style;

void opengl_init (vector2 size);
void opengl_clear (vector2 size, lc_color color);
void opengl_rect (lc_rect rect, lc_color color);
void opengl_image (lc_rect rect, lc_color color, lc_image image);
void opengl_text (lc_rect rect, lc_color color, lc_color shadow_color, lc_font font, char* text, align_style alignment);

#endif