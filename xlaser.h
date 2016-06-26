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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
//#include <X11/Xft/Xft.h>
#include <X11/extensions/Xdbe.h>

#include "xfds.h"

volatile sig_atomic_t abort_signaled = 0;

typedef struct /*_XDATA*/ {
	int screen;
	Display* display;
	Window main;
	XdbeBackBuffer back_buffer;
	Atom wm_delete;
	X_FDS xfds;
} XRESOURCES;

typedef struct /*_GOBO*/ {
	int width;
	int height;
	int components;
	XImage* ximage;
	uint8_t* data;
} GOBO_IMG;

#define DMX_CHANNELS 16
typedef struct /*XLASER_CFG*/ {
	uint16_t dmx_address;
	uint8_t dmx_channels[DMX_CHANNELS];
	uint8_t art_net;
	uint8_t art_subUni;
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
	GOBO_IMG gobo[255];
	//char* backgnd_image;
	//chanmap?
} CONFIG;


// CHANNELS
// X 16bit		0 1
// Y 16bit		2 3
// R G B 24bit		4 5 6
// Dimmer		7
// Shutter		8
// Gobo 		9
// Focus
// Rotation Abs
// Rotation Speed
#include "easy_config.h"
#include "easy_config.c"
#include "network.h"
#include "artnet.h"
#include "artnet.c"
#include "xfds.c"
#include "x11.c"
#include "coreloop.c"
