void generate_gauss_filter(XRESOURCES* res) {
	//interpolate blur kernel
	int i;
	int j;
	double first = 1.0 / (2.0 * M_PI * BLUR_SIGMA * BLUR_SIGMA);
	double div = 2.0 * BLUR_SIGMA * BLUR_SIGMA;
	double val;
	double sum = 0.0;

	int half_dim = BLUR_KERNEL_DIM / 2;

	// generate gauss values
	for (i = half_dim * (-1.0); i < half_dim + 1; i++) {
		for (j = (-1.0) * half_dim; j < half_dim + 1; j++) {
			val = (first * exp( -1.0 * ((i * i + j * j) / div)));
			sum += val;
			res->gauss_kernel[i + half_dim][j + half_dim] = val;
		}
	}

	// normalize
	for (i = 0; i < BLUR_KERNEL_DIM; i++) {
		for (j = 0; j < BLUR_KERNEL_DIM; j++) {
			res->gauss_kernel[i][j] /= sum;
		}
	}
}

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
	return 0;
}

int x11_render(XRESOURCES* xres, uint8_t* channels){
	fprintf(stderr, "begin rendering\n");
	uint8_t selected_gobo;
	struct timespec current_time = {};
	//XColor rgb_color;
	XRenderColor render_color;
	XTransform transform = {{
		{XDoubleToFixed(1), XDoubleToFixed(0), XDoubleToFixed(0)},
		{XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0)},
		{XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1)}
	}};

	//legacy compliant code
	/*XGCValues debug_gc_values = {
		.foreground = WhitePixel(xres->display, xres->screen),
		.background = BlackPixel(xres->display, xres->screen)
	};
	GC debug_gc;*/
	//presentation
	double scaling_factor = 1.0;
	double dimmer_factor = 1.0;
	double angle = 0.0;
	double angle_sin = 0.0, angle_cos = 0.0;
	unsigned x_pos, y_pos;
	long shutter_offset = 0, shutter_interval = 0;

	//XdbeSwapInfo swap_info;

	//set up gobo
	//FIXME might only want to update this upon incoming DMX data
	for(selected_gobo = channels[GOBO]; !(xres->gobo[selected_gobo].data) && selected_gobo >= 0; selected_gobo--){
	}

	if(!(xres->gobo[selected_gobo].data)){
		fprintf(stderr, "No gobo selection possible\n");
		return -1;
	}

	//read color data
	render_color.red = channels[RED] << 8;
	render_color.green = channels[GREEN] << 8;
	render_color.blue = channels[BLUE] << 8;
	//FIXME is this still needed
	/*rgb_color.red = channels[RED] << 8;
	rgb_color.green = channels[GREEN] << 8;
	rgb_color.blue = channels[BLUE] << 8;
	XAllocColor(xres->display, xres->colormap, &rgb_color);
	debug_gc_values.foreground = rgb_color.pixel;
	//XChangeGC(xres->display, debug_gc, GCForeground, &debug_gc_values);*/
	
	//calculate dimmer from input data
	dimmer_factor = (double)channels[DIMMER] / 255.0;
	//FIXME using the dimmer channel as alpha component does not seem to work with XRenderFillRectangle
	//render_color.alpha = channels[DIMMER] << 8;

	//process shutter channel
	if(channels[SHUTTER]){
		if(channels[SHUTTER] >= 1 && channels[SHUTTER] <= 127){
			//strobe effect
			shutter_offset = xres->last_render.tv_nsec % (long)(1e9 / (channels[SHUTTER] / 8.0));
			dimmer_factor = (shutter_offset > 55e6) ? 0:dimmer_factor;
		}
		else if(channels[SHUTTER] >= 128 && channels[SHUTTER] <= 192){
			//flash in
			shutter_interval = 1e9 / (channels[SHUTTER] - 127);
			shutter_offset = xres->last_render.tv_nsec % shutter_interval;
			dimmer_factor *= (double) shutter_offset / (double) shutter_interval;
		}
		else if(channels[SHUTTER] >= 193 && channels[SHUTTER] <= 255){
			//flash out
			shutter_interval = 1e9 / (channels[SHUTTER] - 192);
			shutter_offset = xres->last_render.tv_nsec % shutter_interval;
			dimmer_factor *= 1.0 - ((double) shutter_offset / (double) shutter_interval);
		}
	}

	//apply dimming
	render_color.red *= dimmer_factor;
	render_color.green *= dimmer_factor;
	render_color.blue *= dimmer_factor;

	//set up zoom
	scaling_factor = (double)(256 - channels[ZOOM]) / 255.0;
	if(scaling_factor > 1.0f){
		scaling_factor = 1.0f;
	}
	
	//set up position
	x_pos = ((double)((channels[PAN] << 8) | channels[PAN_FINE]) / 65535.0) * ((double)xres->window_width - ((double)xres->gobo[selected_gobo].width * scaling_factor));
	y_pos = ((double)((channels[TILT] << 8) | channels[TILT_FINE]) / 65535.0) * ((double)xres->window_height - ((double)xres->gobo[selected_gobo].height * scaling_factor));
	//fprintf(stderr, "PAN/FINE: %d/%d, W: %d, X: %d\n", config->dmx_channels[PAN], config->dmx_channels[PAN_FINE], window_width, x_pos);
	//fprintf(stderr, "TILT/FINE: %d/%d, H: %d, Y: %d\n", config->dmx_channels[TILT], config->dmx_channels[TILT_FINE], window_height, y_pos);

	//correct position for scaled gobo dimensions
	x_pos -= (double)(xres->gobo[selected_gobo].width - ((double)xres->gobo[selected_gobo].width * scaling_factor)) / 2.0;
	y_pos -= (double)(xres->gobo[selected_gobo].height - ((double)xres->gobo[selected_gobo].height * scaling_factor)) / 2.0;
	
	//set up rotation
	angle = M_PI / 180 * ((double)(channels[ROTATION]) / 255.0) * 360;
	angle_sin = sin(angle);
	angle_cos = cos(angle);
	
	double center_x = (double)xres->gobo[selected_gobo].width / 2.0;
	double center_y = (double)xres->gobo[selected_gobo].height / 2.0;

	double transform_x = -center_x * angle_cos + -center_y * angle_sin + scaling_factor * center_x;
	double transform_y = center_x * angle_sin + -center_y * angle_cos + scaling_factor * center_y;

	//flood-fill the color pixmap TODO benchmark XFillRectangle vs XRenderFillRectangle
	//XFillRectangle(xres->display, color_pixmap, debug_gc, 0, 0, window_width, window_height);
	XRenderFillRectangle(xres->display, PictOpSrc, xres->color_buffer, &render_color, 0, 0, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);

	//fill color pixmap via normal X operation
	//XFillRectangle(xres->display, xres->back_buffer, debug_gc, 200, 200, 50, 50);
	//XRenderFillRectangle(xres->display, PictOpOver, back_buffer, &render_color, 400, 200, 50, 50);
	//XPutImage(xres->display, xres->back_buffer, DefaultGC(xres->display, xres->screen), xres->gobo[selected_gobo].ximage, 0, 0, x_pos, y_pos, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);
	
	XPutImage(xres->display, xres->gobo_pixmap, xres->window_gc, xres->gobo[selected_gobo].ximage, 0, 0, 0, 0, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);
	//XPutImage(xres->display, xres->gobo_pixmap, DefaultGC(xres->display, xres->screen), xres->gobo[selected_gobo].ximage, 0, 0, 0, 0, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);
	//XPutImage(xres->display, xres->back_buffer, debug_gc, xres->gobo[selected_gobo].ximage, 0, 0, x_pos, y_pos, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);

	//TODO fix XRender double buffering
	/*if(config->double_buffer){
		fprintf(stderr, "Swapping buffers\n");
		swap_info.swap_window = xres->main;
		swap_info.swap_action = XdbeBackground;
		XdbeSwapBuffers(xres->display, &swap_info, 1);
	}
	else{
		fprintf(stderr, "Clearing window\n");
		XClearWindow(xres->display, xres->main);
	}*/

	//set up the transform matrix
	transform.matrix[2][2] = XDoubleToFixed(scaling_factor);
	
	transform.matrix[0][0] = XDoubleToFixed(angle_cos);
	transform.matrix[0][1] = XDoubleToFixed(angle_sin);
	transform.matrix[1][0] = XDoubleToFixed(-angle_sin);
	transform.matrix[1][1] = XDoubleToFixed(angle_cos);

	transform.matrix[0][2] = XDoubleToFixed(transform_x);
	transform.matrix[1][2] = XDoubleToFixed(transform_y);

	if(xres->blur_enabled){
		if(channels[FOCUS]){
			// n x n matrix plus width and height
			int blur_kernel_size = BLUR_KERNEL_DIM * BLUR_KERNEL_DIM + 2;
			XFixed blur_kernel[blur_kernel_size];

			blur_kernel[0] = XDoubleToFixed(BLUR_KERNEL_DIM);
			blur_kernel[1] = XDoubleToFixed(BLUR_KERNEL_DIM);

			double chan = (double) channels[FOCUS] / 255.0;
			double val = 0.0;

			unsigned i, j;

			for (i = 0; i < BLUR_KERNEL_DIM; i++) {
				for (j = 0; j < BLUR_KERNEL_DIM; j++) {
					val = chan * xres->gauss_kernel[i][j];
					if (i == BLUR_KERNEL_DIM / 2 && j == BLUR_KERNEL_DIM / 2) {
						val += 1.0 - chan;
					}
					blur_kernel[(i * BLUR_KERNEL_DIM) + j + 2] = XDoubleToFixed(val);
				}
			}
			fprintf(stderr, "Applying blur filter\n");
			//apply blur
			XRenderSetPictureFilter(xres->display, xres->alpha_mask, "convolution", blur_kernel, blur_kernel_size);
		}
		else{
			XRenderSetPictureFilter(xres->display, xres->alpha_mask, "fast", NULL, 0);
		}
	}

	XRenderSetPictureTransform(xres->display, xres->alpha_mask, &transform);
	//XRenderSetPictureTransform(xres->display, color_buffer, &transform);
	XClearWindow(xres->display, xres->main);

	XRenderComposite(xres->display, PictOpOver, xres->color_buffer, xres->alpha_mask, xres->composite_buffer, 0, 0, 0, 0, x_pos, y_pos, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);
	//XRenderComposite(xres->display, PictOpOver, alpha_mask, alpha_mask, color_buffer, 0, 0, 0, 0, 0, 0, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);
	//XRenderComposite(xres->display, PictOpOver, color_buffer, None, back_buffer, 0, 0, 0, 0, x_pos, y_pos, xres->gobo[selected_gobo].width, xres->gobo[selected_gobo].height);
	//XRenderFillRectangle(xres->display, PictOpSrc, back_buffer, &render_color, 600, 200, 50, 50);
	
	//set the last render timer and update the shutter offset
	if(clock_gettime(CLOCK_MONOTONIC_RAW, &current_time)){
		perror("clock_gettime");
	}

	long rendertime = current_time.tv_nsec - xres->last_render.tv_nsec;
	rendertime = rendertime < 0 ? 1e9 - rendertime:rendertime;
	fprintf(stderr, "Render time %ld, %f rps\n", rendertime, 1e9/rendertime);

	xres->last_render = current_time;
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
