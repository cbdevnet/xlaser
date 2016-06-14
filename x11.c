int x11_init(XRESOURCES* res, CONFIG* config){
	Window root;
	XSetWindowAttributes window_attributes;
	unsigned width, height;
	Atom wm_state_fullscreen;
	int xdbe_major, xdbe_minor;
	XTextProperty window_name;
	pid_t pid = getpid();

	//allocate some structures
	XSizeHints* size_hints = XAllocSizeHints();
	XWMHints* wm_hints = XAllocWMHints();
	XClassHint* class_hints = XAllocClassHint();

	if(!size_hints || !wm_hints || !class_hints){
		fprintf(stderr, "Failed to allocate X data structures\n");
		return -1;
	}

	//x data initialization
	res->display = XOpenDisplay(NULL);

	if(!(res->display)){
		fprintf(stderr, "Failed to open display\n");
		XFree(size_hints);
		XFree(wm_hints);
		XFree(class_hints);
		return -1;
	}

	config->double_buffer = (XdbeQueryExtension(res->display, &xdbe_major, &xdbe_minor) != 0);
	res->screen = DefaultScreen(res->display);
	root = RootWindow(res->display, res->screen);

	//set up window params
	window_attributes.background_pixel = XBlackPixel(res->display, res->screen);
	window_attributes.cursor = None;
	window_attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
	width = DisplayWidth(res->display, res->screen);
	height = DisplayHeight(res->display, res->screen);

	//create window
	res->main = XCreateWindow(res->display,
				root,
				0,
				0,
				width,
				height,
				0,
				CopyFromParent,
				InputOutput,
				CopyFromParent,
				CWBackPixel | CWCursor | CWEventMask,
				&window_attributes);

	//set window properties
	if(XStringListToTextProperty(&(config->window_name), 1, &window_name) == 0){
		fprintf(stderr, "Failed to create string list, aborting\n");
		return -1;
	}

	wm_hints->flags = 0;
	class_hints->res_name = "xlaser";
	class_hints->res_class="xlaser";

	XSetWMProperties(res->display, res->main, &window_name, NULL, NULL, 0, NULL, wm_hints, class_hints);

	XFree(window_name.value);
	XFree(size_hints);
	XFree(wm_hints);
	XFree(class_hints);

	//set fullscreen mode
	if(!config->windowed){
		wm_state_fullscreen = XInternAtom(res->display, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(res->display, res->main, XInternAtom(res->display, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (unsigned char*) &wm_state_fullscreen, 1);
	}

	XChangeProperty(res->display, res->main, XInternAtom(res->display, "_NET_WM_PID", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&pid, 1);

	//allocate back drawing buffer
	if(config->double_buffer){
		res->back_buffer = XdbeAllocateBackBufferName(res->display, res->main, XdbeBackground);
	}

	//register for WM_DELETE_WINDOW messages
	res->wm_delete = XInternAtom(res->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(res->display, res->main, &(res->wm_delete), 1);

	//map window
	XMapRaised(res->display, res->main);

	//get x socket fds
	if(!xfd_add(&(res->xfds), XConnectionNumber(res->display))){
		fprintf(stderr, "Failed to allocate xfd memory\n");
		return -1;
	}
	if(XAddConnectionWatch(res->display, xconn_watch, (void*)(&(res->xfds))) == 0){
		fprintf(stderr, "Failed to register connection watch procedure\n");
		return -1;
	}

	return 0;
}
