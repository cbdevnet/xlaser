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
#define DMX_CHANNELS 16

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifdef OPENGL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#else
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xrender.h>
#define BLUR_KERNEL_DIM 3
#define BLUR_SIGMA 20.0
#define BLUR_CONSTANT 4
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

#ifdef OPENGL
typedef struct /*PROGRAM IDs*/{
	GLuint program;
	GLuint attribute;
	GLuint sampler[2];
	GLuint color;
	GLuint modelview;
	GLuint horizontal;
	GLuint exposure;
}PROGRAM_ID;
#endif

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
	PROGRAM_ID program_id[4];
	GLuint gobo_texture_ID;
	uint8_t gobo_last;
	#endif
} XRESOURCES;

typedef struct /*_CHANNEL_CFG*/ {
	bool fixed;
	bool inverted;
	uint16_t source;
	uint8_t min;
	uint8_t max;
} CHANNEL_CONFIG;

typedef struct /*XLASER_CFG*/ {
	uint16_t dmx_address;
	uint8_t dmx_data[DMX_CHANNELS];
	CHANNEL_CONFIG dmx_config[DMX_CHANNELS];
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
	FOCUS = 13,
	EXPOSURE = 14
};

const char* CHANNEL_NAME[DMX_CHANNELS] = {
	[PAN] = "pan",
	[PAN_FINE] = "panfine",
	[TILT] = "tilt",
	[TILT_FINE] = "tiltfine",
	[RED] = "red",
	[GREEN] = "green",
	[BLUE] = "blue",
	[DIMMER] = "dimmer",
	[SHUTTER] = "shutter",
	[GOBO] = "gobo",
	[ZOOM] = "zoom",
	[ROTATION] = "rotation",
	[ROTATION_SPEED] = "rotationspeed",
	[FOCUS] = "focus",
	[EXPOSURE] = "exposure"
};

int usage(char* fn);

#include "easy_config.h"
#include "easy_config.c"
#include "easy_args.h"
#include "easy_args.c"
#include "config.c"
#include "network.c"
#include "artnet.h"
#include "artnet.c"
#include "xfds.c"
#ifndef OPENGL
#include "backend_xrender.c"
#else
#include "shaders/shaders.h"
#include "backend_opengl.c"
#endif
#include "x11.c"
#include "coreloop.c"
