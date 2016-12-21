int x11_init(XRESOURCES* res, CONFIG* config){
	Window root;
	XSetWindowAttributes window_attributes;
	Atom wm_state_fullscreen;
	XTextProperty window_name;
	pid_t pid = getpid();
	unsigned u;
	char* gobo_path = NULL;

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

	//XSynchronize(res->display, True);

	res->screen = DefaultScreen(res->display);
	root = RootWindow(res->display, res->screen);

	if(!XMatchVisualInfo(res->display, res->screen, 32, TrueColor, &(res->visual_info))) {
		fprintf(stderr, "Display does not support RGBA TrueColor visual\n");
		return -1;
	}

	res->colormap = XCreateColormap(res->display, root, res->visual_info.visual, AllocNone);

	//set up window params
	window_attributes.background_pixel = XBlackPixel(res->display, res->screen);
	window_attributes.border_pixel = XBlackPixel(res->display, res->screen);
	window_attributes.colormap = res->colormap;
	window_attributes.cursor = None;
	window_attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
	res->window_width = DisplayWidth(res->display, res->screen);
	res->window_height = DisplayHeight(res->display, res->screen);

	if(config->windowed){
		res->window_width = config->window_width;
		res->window_height = config->window_height;
	}

	//create window
	res->main = XCreateWindow(res->display,
				root,
				0,
				0,
				res->window_width,
				res->window_height,
				0,
				32,
				InputOutput,
				res->visual_info.visual,
				CWBackPixel | CWCursor | CWEventMask | CWBorderPixel | CWColormap,
				&window_attributes);

	//set window properties
	if(XStringListToTextProperty(&(config->window_name), 1, &window_name) == 0){
		fprintf(stderr, "Failed to create string list, aborting\n");
		return -1;
	}

	wm_hints->flags = 0;
	class_hints->res_name = "xlaser";
	class_hints->res_class = "xlaser";

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

	//prepare image data
	//FIXME TODO this whole section needs error checks
	gobo_path = calloc(strlen(config->gobo_prefix) + 9, sizeof(char));
	if(!gobo_path){
		fprintf(stderr, "Failed to allocate memory for gobo search path\n");
		return -1;
	}
	strcpy(gobo_path, config->gobo_prefix);

	for(u = 0; u < 256; u++){
		res->gobo[u].height = 0;
		res->gobo[u].width = 0;
		snprintf(gobo_path + strlen(config->gobo_prefix), 8, "%d.png", u);
		res->gobo[u].data = stbi_load(gobo_path, &(res->gobo[u].width), &(res->gobo[u].height), &(res->gobo[u].components), 4);

		if(res->gobo[u].data){
			fprintf(stderr, "Gobo %d: %s %dx%d @ %d\n", u, gobo_path, res->gobo[u].width, res->gobo[u].height, res->gobo[u].components);
		}
	}
	free(gobo_path);

	return backend_init(res, config);
}

void x11_cleanup(XRESOURCES* res){
	unsigned u;
	if(!res->display){
		return;
	}
	
	backend_free(res);

	for(u = 0; u < 256; u++){
		if(res->gobo[u].data){
			free(res->gobo[u].data);
		}
	}

	if(res->main){
		XDestroyWindow(res->display, res->main);
	}

	if(res->colormap){
		XFreeColormap(res->display, res->colormap);
	}

	XCloseDisplay(res->display);
	xfd_free(&(res->xfds));
}
