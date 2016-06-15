#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
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

#define DMX_CHANNELS 16
typedef struct /*XLASER_CFG*/ {
	uint16_t dmx_address;
	bool windowed;
	unsigned window_width;
	unsigned window_height;
	unsigned x_offset;
	unsigned y_offset;
	unsigned xmax;
	unsigned ymax;
	char* bindhost;
	char* window_name;
	bool double_buffer;
	int sockfd;
	uint8_t dmx_channels[DMX_CHANNELS];
	//char* backgnd_image;
	//char** gobos;
	//chanmap?
} CONFIG;


// CHANNELS
// X 16bit
// Y 16bit
// R G B 24bit
// Dimmer
// Shutter
// Gobo
// Focus
// Rotation Abs
// Rotation Speed

#include "network.h"
#include "artnet.h"
#include "artnet.c"
#include "xfds.c"
#include "x11.c"
#include "coreloop.c"
