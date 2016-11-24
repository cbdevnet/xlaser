int backend_init(XRESOURCES* res, CONFIG* config){
	#ifdef OPENGL
	res->gl_context = glXCreateContext(res->display, &visual_info, NULL, True);
	#endif
	return 0;
}