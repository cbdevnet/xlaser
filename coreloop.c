int xlaser(XRESOURCES* xres, CONFIG* config){
	fd_set readfds;
	struct timeval tv;
	int maxfd, error;
	unsigned i;
	XEvent event;
	bool exposed, reconfigured;
	struct timespec current_time = {};

	xres->window_width = DisplayWidth(xres->display, xres->screen);
	xres->window_height = DisplayHeight(xres->display, xres->screen);

	char pressed_key;

	while(!abort_signaled){
		exposed = reconfigured = false;
		//handle events
		while(XPending(xres->display)){
			XNextEvent(xres->display, &event);
			//handle events
			switch(event.type){
				case ConfigureNotify:
					fprintf(stderr, "Window configured to %dx%d\n", event.xconfigure.width, event.xconfigure.height);
					xres->window_width = event.xconfigure.width;
					xres->window_height = event.xconfigure.height;
					reconfigured = true;
					break;

				case Expose:
					exposed = true;
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

		if(reconfigured){
			if(xlaser_reconfigure(xres)){
				fprintf(stderr, "Backend failed to reconfigure\n");
			}
		}

		if(exposed || config->dmx_channels[SHUTTER] != 0){
			fprintf(stderr, "Window exposed, drawing\n");
			//FIXME this might loop
			if(xlaser_render(xres, config->dmx_channels) < 0){
				fprintf(stderr, "Render procedure failed\n");
			}
			//set the last render timer
			if(clock_gettime(CLOCK_MONOTONIC_RAW, &current_time)){
				perror("clock_gettime");
			}
			xres->last_render = current_time;
		}

		XFlush(xres->display);

		if(abort_signaled){
			break;
		}

		//prepare select data
		FD_ZERO(&readfds);
		maxfd = -1;
		//when using the shutter feature, cycle faster
		tv.tv_sec = (config->dmx_channels[SHUTTER] == 0) ? 1:0;
		tv.tv_usec = (config->dmx_channels[SHUTTER] == 0) ? 0:1000;

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
				artnet_handler(config);
				event.type = Expose;
				XSendEvent(xres->display, xres->main, False, 0, &event);
			}
			else{
			}
		}
		else if(error < 0){
			perror("select");
			abort_signaled = -1;
		}
	}

	//free data

	//TODO XrenderFreePicture
	//XFreePixmap
	//XDestroyImage

	return abort_signaled;
}
