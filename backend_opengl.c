int backend_init(XRESOURCES* res, CONFIG* config){
	res->gl_context = glXCreateContext(res->display, &(res->visual_info), NULL, True);
	return 0;
}

int xlaser_render(XRESOURCES* xres, uint8_t* channels){
	return 0;
}

void backend_free(XRESOURCES* res){
}
