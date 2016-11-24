int x11_init(XRESOURCES* res, CONFIG* config){
	Window root;
	XSetWindowAttributes window_attributes;
	unsigned width, height;
	Atom wm_state_fullscreen;
	int xdbe_major, xdbe_minor;
	int xrender_major, xrender_minor;
	XTextProperty window_name;
	XVisualInfo visual_info;
	pid_t pid = getpid();
	unsigned u;
	char* gobo_path = NULL;
	int gobo_max_width = 0, gobo_max_height = 0;

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

	if(!XRenderQueryExtension(res->display, &xrender_major, &xrender_minor)){
		fprintf(stderr, "XRender extension not enabled on display\n");
		return -1;
	}
	XRenderQueryVersion(res->display, &xrender_major, &xrender_minor);
	fprintf(stderr, "Xrender version %d.%d\n", xrender_major, xrender_minor);

	config->double_buffer = (XdbeQueryExtension(res->display, &xdbe_major, &xdbe_minor) != 0);
	res->screen = DefaultScreen(res->display);
	root = RootWindow(res->display, res->screen);
	fprintf(stderr, "Xdbe version %d.%d\n", xdbe_major, xdbe_minor);

	if(!XMatchVisualInfo(res->display, res->screen, 32, TrueColor, &visual_info)) {
		fprintf(stderr, "Display does not support RGBA TrueColor visual\n");
		return -1;
	}

	res->colormap = XCreateColormap(res->display, root, visual_info.visual, AllocNone);

	//set up window params
	window_attributes.background_pixel = XBlackPixel(res->display, res->screen);
	window_attributes.border_pixel = XBlackPixel(res->display, res->screen);
	window_attributes.colormap = res->colormap;
	window_attributes.cursor = None;
	window_attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
	width = DisplayWidth(res->display, res->screen);
	height = DisplayHeight(res->display, res->screen);

	if(config->windowed){
		width = config->window_width;
		height = config->window_height;
	}

	//create window
	res->main = XCreateWindow(res->display,
				root,
				0,
				0,
				width,
				height,
				0,
				32,
				InputOutput,
				visual_info.visual,
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
			//res->gobo[u].ximage = XCreateImage(res->display, vinfo.visual, vinfo.depth, ZPixmap, 0, (char*)res->gobo[u].data, res->gobo[u].width, res->gobo[u].height, 32, 0);
			//res->gobo[u].ximage = XCreateImage(res->display, DefaultVisual(res->display, res->screen), DefaultDepth(res->display, res->screen), ZPixmap, 0, (char*)res->gobo[u].data, res->gobo[u].width, res->gobo[u].height, 32, 0);
			res->gobo[u].ximage = XCreateImage(res->display, DefaultVisual(res->display, res->screen), 32, ZPixmap, 0, (char*)res->gobo[u].data, res->gobo[u].width, res->gobo[u].height, 32, 0);
			
			if(res->gobo[u].height > gobo_max_height){
				gobo_max_height = res->gobo[u].height;
			}
			if(res->gobo[u].width > gobo_max_width){
				gobo_max_width = res->gobo[u].width;
			}
			
			if(!res->gobo[u].ximage){
				fprintf(stderr, "Failed to create XImage for gobo %d\n", u);
				return -1;
			}
		}
	}
	free(gobo_path);
	
	fprintf(stderr, "Creating gobo pixmaps with dimensions %dx%d\n", gobo_max_width, gobo_max_height);
	res->gobo_pixmap = XCreatePixmap(res->display, res->back_buffer, gobo_max_width, gobo_max_height, 32);
	res->color_pixmap = XCreatePixmap(res->display, res->back_buffer, gobo_max_width, gobo_max_height, 32);
	if(!res->gobo_pixmap || ! res->color_pixmap){
		fprintf(stderr, "Failed to create backing pixmaps\n");
		return -1;
	}

	//debug_gc = XCreateGC(xres->display, gobo_pixmap, GCForeground | GCBackground, &debug_gc_values);
	res->window_gc = XCreateGC(res->display, res->gobo_pixmap, 0, NULL);
	res->composite_buffer = XRenderCreatePicture(res->display, res->main, XRenderFindStandardFormat(res->display, PictStandardARGB32), 0, 0);
	res->color_buffer = XRenderCreatePicture(res->display, res->color_pixmap, XRenderFindStandardFormat(res->display, PictStandardARGB32), 0, 0);
	res->alpha_mask = XRenderCreatePicture(res->display, res->gobo_pixmap, XRenderFindStandardFormat(res->display, PictStandardARGB32), 0, 0);

	//check XRender filtering capabilities
	XFilters* filters = XRenderQueryFilters(res->display, res->gobo_pixmap);
	for(u = 0; u < filters->nfilter; u++){
		fprintf(stderr, "Available filter %d of %d: %s\n", u + 1, filters->nfilter, filters->filter[u]);
		if(!strcmp(filters->filter[u], "convolution")){
			fprintf(stderr, "Convolution filter supported, enabling focus effect...\n");
			res->blur_enabled = true;
			generate_gauss_filter(res);
		}
	}
	XFree(filters);

	#ifdef OPENGL
	res->gl_context = glXCreateContext(res->display, &visual_info, NULL, True);
	#endif
	return 0;
}

void x11_cleanup(XRESOURCES* res){
	unsigned u;
	if(!res->display){
		return;
	}

	XRenderFreePicture(res->display, res->composite_buffer);
	XRenderFreePicture(res->display, res->alpha_mask);
	XRenderFreePicture(res->display, res->color_buffer);

	XFreePixmap(res->display, res->gobo_pixmap);
	XFreePixmap(res->display, res->color_pixmap);

	for(u = 0; u < 256; u++){
		if(res->gobo[u].data){
			//XDestroyImage also frees the backing data, so stbi_image_free would be a double-free
			XDestroyImage(res->gobo[u].ximage);
		}
	}

	XFreeGC(res->display, res->window_gc);

	if(res->main){
		XDestroyWindow(res->display, res->main);
	}

	XCloseDisplay(res->display);
	xfd_free(&(res->xfds));
}
