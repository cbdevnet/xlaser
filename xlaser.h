#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <math.h>

#define XLASER_VERSION "XLaser v1.1"
#define SHORTNAME "XLaser"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifndef OPENGL
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xrender.h>
#define BLUR_KERNEL_DIM 3
#define BLUR_SIGMA 20.0
#define BLUR_CONSTANT 4
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "xfds.h"

volatile sig_atomic_t abort_signaled = 0;


typedef struct /*_GOBO*/ {
	int width;
	int height;
	int components;
	#ifndef OPENGL
	XImage* ximage;
	#endif
	uint8_t* data;
} GOBO_IMG;

typedef struct /*_XDATA*/ {
	int screen;
	Display* display;
	Window main;
	X_FDS xfds;
	Atom wm_delete;
	unsigned window_width;
	unsigned window_height;
	GOBO_IMG gobo[256];
	struct timespec last_render;
	Colormap colormap;
	XVisualInfo visual_info;
	#ifndef OPENGL
	XdbeBackBuffer back_buffer;
	Pixmap gobo_pixmap;
	Pixmap color_pixmap;
	Picture composite_buffer;
	Picture alpha_mask;
	Picture color_buffer;
	bool blur_enabled;
	GC window_gc;
	double gauss_kernel[BLUR_KERNEL_DIM][BLUR_KERNEL_DIM];
	#else
	GLXContext gl_context;
	GLuint fboID[2];
	GLuint rbo_depth[2];
	GLuint fbo_texture[2];
	GLuint fbo_vbo_ID;
	GLuint fbo_program_ID;
	GLuint fbo_program_texture_sampler;
	GLuint fbo_program_filter;
	GLuint fbo_program_horizontal;
	GLuint fbo_program_attribute;
	GLuint gobo_texture_ID;
	GLuint gobo_program_ID;
	GLuint gobo_program_texture_sampler;
	GLuint gobo_program_colormod;
	GLuint gobo_program_attribute;
	GLuint gobo_modelview_ID;
	uint8_t gobo_last;
	GLuint light_program_ID;
	GLuint light_modelview_ID;
	GLuint light_program_texture_sampler;
	GLuint light_program_attribute;
	GLuint light_program_colormod;
	#endif
} XRESOURCES;

#define DMX_CHANNELS 16
typedef struct /*XLASER_CFG*/ {
	uint16_t dmx_address;
	uint8_t dmx_channels[DMX_CHANNELS];
	uint8_t art_net;
	uint8_t art_subUni;
	uint8_t art_universe;
	bool windowed;
	unsigned window_width;
	unsigned window_height;
	unsigned x_offset;
	unsigned y_offset;
	unsigned xmax;
	unsigned ymax;
	char* bindhost;
	char* window_name;
	char* gobo_prefix;
	bool double_buffer;
	int sockfd;
	//char* backgnd_image;
	//chanmap?
} CONFIG;

enum /*DMX_CHANNEL*/ {
	PAN = 0,
	PAN_FINE = 1,
	TILT = 2,
	TILT_FINE = 3,
	RED = 4,
	GREEN = 5,
	BLUE = 6,
	DIMMER = 7,
	SHUTTER = 8,
	GOBO = 9,
	ZOOM = 10,
	ROTATION = 11,
	ROTATION_SPEED = 12,
	FOCUS = 13
};

int usage(char* fn);

#include "easy_config.h"
#include "easy_config.c"
#include "easy_args.h"
#include "easy_args.c"
#include "config.c"
#include "network.h"
#include "artnet.h"
#include "artnet.c"
#include "xfds.c"
#ifndef OPENGL
#include "backend_xrender.c"
#else
#include "shaders/filter_fragment.h"
#include "shaders/filter_vertex.h"
#include "shaders/gobo_fragment.h"
#include "shaders/gobo_vertex.h"
#include "shaders/light_fragment.h"
#include "shaders/light_vertex.h"
#include "backend_opengl.c"
#endif
#include "x11.c"
#include "coreloop.c"
