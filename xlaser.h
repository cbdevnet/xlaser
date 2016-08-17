#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//#include <unistd.h>
//#include <fcntl.h>
//#include <ctype.h>
//#include <math.h>
//#include <errno.h>

#define XLASER_VERSION "XLaser v1.1"
#define SHORTNAME "XLaser"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
//#include <X11/Xft/Xft.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xrender.h>

#include "xfds.h"

volatile sig_atomic_t abort_signaled = 0;

typedef struct /*_GOBO*/ {
	int width;
	int height;
	int components;
	XImage* ximage;
	uint8_t* data;
} GOBO_IMG;

typedef struct /*_XDATA*/ {
	int screen;
	Display* display;
	Window main;
	XdbeBackBuffer back_buffer;
	Atom wm_delete;
	X_FDS xfds;
	Colormap colormap;
	Pixmap gobo_pixmap;
	Pixmap color_pixmap;
	Pixmap dimmer_pixmap;
	Picture composite_buffer;
	Picture alpha_mask;
	Picture color_buffer;
	Picture dimmer_mask;
	GC window_gc;
	unsigned window_width;
	unsigned window_height;
	GOBO_IMG gobo[255];
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
	ROTATION_SPEED = 12
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
#include "x11.c"
#include "coreloop.c"
