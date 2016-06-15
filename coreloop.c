int xlaser(XRESOURCES* xres, CONFIG* config){
	fd_set readfds;
	struct timeval tv;
	int maxfd, error;
	unsigned i;
	XEvent event;
	XdbeSwapInfo swap_info;

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
					break;

				case Expose:
					//draw here
					fprintf(stderr, "Expose message, initiating redraw\n");

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

	return abort_signaled;
}