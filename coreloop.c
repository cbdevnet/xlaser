int xlaser(XRESOURCES* xres, CONFIG* config){
	fd_set readfds;
	struct timeval tv;
	int maxfd, error;
	unsigned i;
	XEvent event;
	XdbeSwapInfo swap_info;
	int window_width, window_height;
	unsigned x_pos, y_pos;
	uint8_t selected_gobo;
	XRenderColor render_color;
	XColor rgb_color;
	Pixmap gobo_pixmap;
	Pixmap color_pixmap;
	GC debug_gc;
	Picture back_buffer;
	Picture alpha_mask;
	Picture color_buffer;
	XGCValues debug_gc_values = {
		.foreground = WhitePixel(xres->display, xres->screen),
		.background = BlackPixel(xres->display, xres->screen)
	};

	XTransform transform = {{
		{XDoubleToFixed(1), XDoubleToFixed(0), XDoubleToFixed(0)},
		{XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0)},
		{XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1)}
	}};

	window_width = DisplayWidth(xres->display, xres->screen);
	window_height = DisplayHeight(xres->display, xres->screen);

	//FIXME do this in x11 init?
	//FIXME only allocate this with the size of the biggest gobo
	int gobo_max_width = 0, gobo_max_height = 0;
	for(i = 0; i < 256; i++){
		if(config->gobo[i].height > gobo_max_height){
			gobo_max_height = config->gobo[i].height;
		}
		if(config->gobo[i].width > gobo_max_width){
			gobo_max_width = config->gobo[i].width;
		}
	}
	fprintf(stderr, "Creating gobo pixmaps with dimensions %dx%d\n", gobo_max_width, gobo_max_height);
	gobo_pixmap = XCreatePixmap(xres->display, xres->back_buffer, gobo_max_width, gobo_max_height, 32);
	color_pixmap = XCreatePixmap(xres->display, xres->back_buffer, gobo_max_width, gobo_max_height, 32);
	if(!gobo_pixmap){
		fprintf(stderr, "Failed to create backing pixmap\n");
		return -1;
	}

	debug_gc = XCreateGC(xres->display, gobo_pixmap, GCForeground | GCBackground, &debug_gc_values);
	back_buffer = XRenderCreatePicture(xres->display, xres->main, XRenderFindStandardFormat(xres->display, PictStandardARGB32), 0, 0);
	color_buffer = XRenderCreatePicture(xres->display, color_pixmap, XRenderFindStandardFormat(xres->display, PictStandardARGB32), 0, 0);
	alpha_mask = XRenderCreatePicture(xres->display, gobo_pixmap, XRenderFindStandardFormat(xres->display, PictStandardARGB32), 0, 0);

	char* display_buffer = NULL;
	char pressed_key;

	while(!abort_signaled){
		//handle events
		while(XPending(xres->display)){
			XNextEvent(xres->display, &event);
			//handle events
			switch(event.type){
				case ConfigureNotify:
					fprintf(stderr, "Window configured to %dx%d\n", event.xconfigure.width, event.xconfigure.height);
					window_width = event.xconfigure.width;
					window_height = event.xconfigure.height;
					break;

				case Expose:
					//draw here
					fprintf(stderr, "Expose message, initiating redraw\n");

					//FIXME this might loop
					for(selected_gobo = config->dmx_channels[GOBO]; !(config->gobo[selected_gobo].data) && selected_gobo >= 0; selected_gobo--){
					}

					if(!(config->gobo[selected_gobo].data)){
						fprintf(stderr, "No gobo selection possible\n");
						break;
					}

					rgb_color.red = config->dmx_channels[RED] << 8;
					rgb_color.green = config->dmx_channels[GREEN] << 8;
					rgb_color.blue = config->dmx_channels[BLUE] << 8;

					render_color.red = config->dmx_channels[RED] << 8;
					render_color.green = config->dmx_channels[GREEN] << 8;
					render_color.blue = config->dmx_channels[BLUE] << 8;
					render_color.alpha = 0xFFFF;

					XAllocColor(xres->display, xres->colormap, &rgb_color);
					debug_gc_values.foreground = rgb_color.pixel;
					XChangeGC(xres->display, debug_gc, GCForeground, &debug_gc_values);
					
					fprintf(stderr, "Using gobo %d\n", selected_gobo);
					x_pos = ((float)((config->dmx_channels[PAN] << 8) | config->dmx_channels[PAN_FINE])/65535.0) * (window_width - config->gobo[selected_gobo].width);
					y_pos = ((float)((config->dmx_channels[TILT] << 8) | config->dmx_channels[TILT_FINE])/65535.0) * (window_height - config->gobo[selected_gobo].height);
					fprintf(stderr, "PAN/FINE: %d/%d, W: %d, X: %d\n", config->dmx_channels[PAN], config->dmx_channels[PAN_FINE], window_width, x_pos);
					fprintf(stderr, "TILT/FINE: %d/%d, H: %d, Y: %d\n", config->dmx_channels[TILT], config->dmx_channels[TILT_FINE], window_height, y_pos);

					//XFillRectangle(xres->display, xres->back_buffer, debug_gc, 200, 200, 50, 50);
					//XRenderFillRectangle(xres->display, PictOpOver, back_buffer, &render_color, 400, 200, 50, 50);
					//XPutImage(xres->display, xres->back_buffer, DefaultGC(xres->display, xres->screen), config->gobo[selected_gobo].ximage, 0, 0, x_pos, y_pos, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);

					//flood-fill the color pixmap TODO benchmark XFillRectangle vs XRenderFillRectangle
					//XFillRectangle(xres->display, color_pixmap, debug_gc, 0, 0, window_width, window_height);
					XRenderFillRectangle(xres->display, PictOpSrc, color_buffer, &render_color, 0, 0, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);
					XPutImage(xres->display, gobo_pixmap, debug_gc, config->gobo[selected_gobo].ximage, 0, 0, 0, 0, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);
					//XPutImage(xres->display, xres->back_buffer, debug_gc, config->gobo[selected_gobo].ximage, 0, 0, x_pos, y_pos, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);

					if(config->double_buffer){
						fprintf(stderr, "Swapping buffers\n");
						swap_info.swap_window = xres->main;
						swap_info.swap_action = XdbeBackground;
						XdbeSwapBuffers(xres->display, &swap_info, 1);
					}
					else{
						fprintf(stderr, "Clearing window\n");
						XClearWindow(xres->display, xres->main);
					}

					double scaling_factor = (double)(256 - config->dmx_channels[ZOOM])/255.0;
					if(scaling_factor > 1.0f){
						scaling_factor = 1.0f;
					}
					fprintf(stderr, "Scaling factor %f\n", scaling_factor);

					transform.matrix[2][2] = XDoubleToFixed(scaling_factor);

					double angle = M_PI / 180 * ((double)(config->dmx_channels[ROTATION])/255.0) * 360;
					double angle_sin = sin(angle);
					double angle_cos = cos(angle);
					fprintf(stderr, "Current angle %f, sine %f, cosine %f\n", angle, angle_sin, angle_cos);

					transform.matrix[0][0] = XDoubleToFixed(angle_cos);
					transform.matrix[0][1] = XDoubleToFixed(angle_sin);
					transform.matrix[1][0] = XDoubleToFixed(-angle_sin);
					transform.matrix[1][1] = XDoubleToFixed(angle_cos);

					double center_x = (double)config->gobo[selected_gobo].width/2.0;
					double center_y = (double)config->gobo[selected_gobo].height/2.0;
					fprintf(stderr, "Centering offset %f:%f\n", center_x, center_y);

					double transform_x = -center_x * angle_cos + -center_y * angle_sin + scaling_factor * center_x;
					double transform_y = center_x * angle_sin + -center_y * angle_cos + scaling_factor * center_y;

					transform.matrix[0][2] = XDoubleToFixed(transform_x);
					transform.matrix[1][2] = XDoubleToFixed(transform_y);

					fprintf(stderr, "Current transform: %d:%d fixed, %f:%f float\n", transform.matrix[0][2], transform.matrix[1][2], transform_x, transform_y);

					//XRenderSetPictureTransform(xres->display, alpha_mask, &transform);
					XRenderSetPictureTransform(xres->display, color_buffer, &transform);

					//XRenderComposite(xres->display, PictOpOver, color_buffer, alpha_mask, back_buffer, 0, 0, 0, 0, x_pos, y_pos, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);
					XRenderComposite(xres->display, PictOpOver, alpha_mask, alpha_mask, color_buffer, 0, 0, 0, 0, 0, 0, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);
					XRenderComposite(xres->display, PictOpOver, color_buffer, None, back_buffer, 0, 0, 0, 0, x_pos, y_pos, config->gobo[selected_gobo].width, config->gobo[selected_gobo].height);
					//XRenderFillRectangle(xres->display, PictOpSrc, back_buffer, &render_color, 600, 200, 50, 50);
					break;

				case KeyPress:
					//translate key event into a character, respecting keyboard layout
					if(XLookupString(&event.xkey, &pressed_key, 1, NULL, NULL) != 1){
						//disregard combined characters / bound strings
						break;
					}
					switch(pressed_key){
						case 'q':
							abort_signaled = 1;
							break;
						case 'r':
							fprintf(stderr, "Redrawing on request\n");
							event.type = Expose;
							XSendEvent(xres->display, xres->main, False, 0, &event);
							break;
						default:
							fprintf(stderr, "KeyPress %d (%c)\n", event.xkey.keycode, pressed_key);
							break;
					}
					break;

				case ClientMessage:
					if(event.xclient.data.l[0] == xres->wm_delete){
						fprintf(stderr, "Closing down window\n");
						abort_signaled = 1;
					}
					else{
						fprintf(stderr, "Client message\n");
					}
					break;

				default:
					fprintf(stderr, "Unhandled X event\n");
					break;
			}
		}

		XFlush(xres->display);

		if(abort_signaled){
			break;
		}

		//prepare select data
		FD_ZERO(&readfds);
		maxfd = -1;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		for(i = 0; i < xres->xfds.size; i++){
			FD_SET(xres->xfds.fds[i], &readfds);
			if(maxfd < xres->xfds.fds[i]){
				maxfd = xres->xfds.fds[i];
			}
		}

		FD_SET(config->sockfd, &readfds);
		if(maxfd < config->sockfd){
			maxfd = config->sockfd;
		}

		error = select(maxfd + 1, &readfds, NULL, NULL, &tv);
		if(error > 0){
			if(FD_ISSET(config->sockfd, &readfds)){
				fprintf(stderr, "ArtNet Data\n");
				artnet_handler(config);
				event.type = Expose;
				XSendEvent(xres->display, xres->main, False, 0, &event);
			}
			else{
				fprintf(stderr, "X Data\n");
			}
		}
		else if(error < 0){
			perror("select");
			abort_signaled = -1;
		}
		else{
			fprintf(stderr, "Nothing\n");
		}
	}

	//free data
	if(display_buffer){
		free(display_buffer);
	}

	//TODO XrenderFreePicture
	//XFreePixmap
	//XDestroyImage

	return abort_signaled;
}
