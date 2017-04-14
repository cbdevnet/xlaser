#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xrender.h>
#define BLUR_KERNEL_DIM 3
#define BLUR_SIGMA 20.0
#define BLUR_CONSTANT 4

typedef struct /*_XRENDER_DATA*/ {
	XdbeBackBuffer back_buffer;
	Pixmap gobo_pixmap;
	Pixmap color_pixmap;
	Picture composite_buffer;
	Picture alpha_mask;
	Picture color_buffer;
	bool blur_enabled;
	GC window_gc;
	double gauss_kernel[BLUR_KERNEL_DIM][BLUR_KERNEL_DIM];
} backend_data;