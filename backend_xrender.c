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

int backend_init(XRESOURCES* res, CONFIG* config){
	int xdbe_major, xdbe_minor;
	int xrender_major, xrender_minor;
	int gobo_max_width = 0, gobo_max_height = 0;
	unsigned u;

	if(!XRenderQueryExtension(res->display, &xrender_major, &xrender_minor)){
		fprintf(stderr, "XRender extension not enabled on display\n");
		return -1;
	}

	config->double_buffer = (XdbeQueryExtension(res->display, &xdbe_major, &xdbe_minor) != 0);
	XRenderQueryVersion(res->display, &xrender_major, &xrender_minor);
	fprintf(stderr, "Xdbe version %d.%d\n", xdbe_major, xdbe_minor);
	fprintf(stderr, "Xrender version %d.%d\n", xrender_major, xrender_minor);

	//allocate back drawing buffer
	if(config->double_buffer){
		res->back_buffer = XdbeAllocateBackBufferName(res->display, res->main, XdbeBackground);
	}

	for(u = 0; u < 256; u++){
		if(res->gobo[u].data){
			res->gobo[u].ximage = XCreateImage(res->display, DefaultVisual(res->display, res->screen), 32, ZPixmap, 0, (char*)res->gobo[u].data, res->gobo[u].width, res->gobo[u].height, 32, 0);
			if(!res->gobo[u].ximage){
				fprintf(stderr, "Failed to create XImage for gobo %d\n", u);
				return -1;
			}

			if(res->gobo[u].height > gobo_max_height){
				gobo_max_height = res->gobo[u].height;
			}
			if(res->gobo[u].width > gobo_max_width){
				gobo_max_width = res->gobo[u].width;
			}
		}
	}

	fprintf(stderr, "Creating gobo pixmaps with dimensions %dx%d\n", gobo_max_width, gobo_max_height);
	res->gobo_pixmap = XCreatePixmap(res->display, res->main, gobo_max_width, gobo_max_height, 32);
	res->color_pixmap = XCreatePixmap(res->display, res->main, gobo_max_width, gobo_max_height, 32);
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

int xlaser_reconfigure(XRESOURCES* res){
	return 0;
}

int xlaser_render(XRESOURCES* xres, uint8_t* channels){
	uint8_t selected_gobo;
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
	return 0;
}

void backend_free(XRESOURCES* res){
	unsigned u;

	XRenderFreePicture(res->display, res->composite_buffer);
	XRenderFreePicture(res->display, res->alpha_mask);
	XRenderFreePicture(res->display, res->color_buffer);

	XFreePixmap(res->display, res->gobo_pixmap);
	XFreePixmap(res->display, res->color_pixmap);

	for(u = 0; u < 256; u++){
		if(res->gobo[u].data){
			//XDestroyImage also frees the backing data, so stbi_image_free would be a double-free
			XDestroyImage(res->gobo[u].ximage);
			res->gobo[u].data = NULL;
		}
	}

	XFreeGC(res->display, res->window_gc);
}
