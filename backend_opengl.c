int backend_compile_shader(GLuint shader_id, unsigned char** shader_source){
	GLint result = GL_FALSE;
	int log_length;
	char* log = NULL;

	glShaderSource(shader_id, 1, (const GLchar**)shader_source, NULL);
	glCompileShader(shader_id);

	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
	if(result != GL_TRUE){
		fprintf(stderr, "Failed to compile shader\n");
		return 1;
	}

	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0){
		log = calloc(log_length + 1, sizeof(char));
		glGetShaderInfoLog(shader_id, log_length, NULL, log);
		fprintf(stderr, "Shader compiler error: %s\n", log);
		free(log);
		return 1;
	}
	return 0;
}

GLuint backend_compile_program(unsigned char* vertex_source, unsigned char* fragment_source){
	//create two shader handles and a program handle
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint program_id = glCreateProgram();

	//compile the shaders
	if(backend_compile_shader(vertex_shader, &vertex_source) 
			|| backend_compile_shader(fragment_shader, &fragment_source)){
		return 0;
	}

	//attach to program
	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);

	glDetachShader(program_id, vertex_shader);
	glDetachShader(program_id, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program_id;
}


int backend_init(XRESOURCES* res, CONFIG* config){
	int major = 0, minor = 0;
	GLenum glew_err;
	//target quad
	GLfloat target_quad[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1
	};

	//create gl context
	res->gl_context = glXCreateContext(res->display, &(res->visual_info), NULL, True);
	if(!(res->gl_context)){
		fprintf(stderr, "Failed to create GL context\n");
		return -1;
	}

	glXMakeContextCurrent(res->display, res->main, res->main, res->gl_context);
	glXQueryVersion(res->display, &major, &minor);
	fprintf(stderr, "OpenGL version %d.%d\n", major, minor);

	//glewExperimental = GL_TRUE;
	glew_err = glewInit();
	if(glew_err != GLEW_OK){
		fprintf(stderr, "Failed to initialize glew: %s\n", glewGetErrorString(err));
		return -1;
	}

	printf("OpenGL implementation: %s\n", glGetString(GL_VERSION));

	glClearColor(0.f, 0.f, 0.f, 1.f);
    	glClear(GL_COLOR_BUFFER_BIT);

    	//Create VAO and ignore it
    	GLuint VertexArrayID;
    	glGenVertexArrays(1, &VertexArrayID);
    	glBindVertexArray(VertexArrayID);

	//enable some features
    	glEnable(GL_TEXTURE_2D);
    	glEnable(GL_MULTISAMPLE);
    	glEnable(GL_BLEND);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//Target texture for render
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &res->fbo_texture);
	glBindTexture( GL_TEXTURE_2D, res->fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res->window_width, res->window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L);

	//RENDER TO TEXTURE
	//Generate Framebuffer and attach Renderbuffer
	glGenFramebuffers(1, &res->fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, res->fboID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res->fbo_texture, 0);

	glGenRenderbuffers(1, &res->rbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, res->rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, res->window_width, res->window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, res->rbo_depth);

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER , 0);

	glGenBuffers(1, &res->fbo_vbo_ID);
	glBindBuffer(GL_ARRAY_BUFFER, res->fbo_vbo_ID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(target_quad), target_quad, GL_STATIC_DRAW);
	//We only use one Arraybuffer, so no need to unbind it.
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	//GOBO Texture
	glGenTextures(1, &res->gobo_texture_ID);
	glBindTexture(GL_TEXTURE_2D, res->gobo_texture_ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//GOBO DATA TO GPU
	//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, res->gobo[0].width, res->gobo[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, res->gobo[0].data );
	glBindTexture(GL_TEXTURE_2D, 0);

	//Get Program ID for Gauss Filter
	res->fbo_program_ID = backend_compile_program(filter_vertex_shader, filter_fragment_shader);
	res->fbo_program_texture_sampler = glGetUniformLocation(res->fbo_program_ID, "textureSampler");
	//res->fbo_program_filter = glGetUniformLocation( res->fbo_program_ID, "filter");
	res->fbo_program_attribute = glGetAttribLocation(res->fbo_program_ID, "vertexCoord");

	//Get Program ID for Gobo
	res->gobo_last = 10;
	res->gobo_program_ID = backend_compile_program(gobo_vertex_shader, gobo_fragment_shader);
	res->gobo_program_texture_sampler = glGetUniformLocation(res->gobo_program_ID, "textureSampler");
	res->gobo_modelview_ID = glGetUniformLocation(res->gobo_program_ID, "modelview");
	res->gobo_program_colormod = glGetUniformLocation(res->gobo_program_ID, "colormod");
	res->gobo_program_attribute = glGetAttribLocation(res->gobo_program_ID, "vertexCoord");
	return 0;
}

int xlaser_reconfigure(XRESOURCES* xres){
	glBindRenderbuffer( GL_RENDERBUFFER, xres->rbo_depth);
	glBindTexture(GL_TEXTURE_2D, xres->fbo_texture);
	glBindFramebuffer(GL_FRAMEBUFFER, xres->fboID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xres->window_width, xres->window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, xres->window_width, xres->window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, xres->rbo_depth);
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return 0;
}

int xlaser_render(XRESOURCES* xres, uint8_t* channels){
	double scaling_factor = (double)(256 - channels[ZOOM]) / 255.0;
	long shutter_offset = 0, shutter_interval = 0;
	double dimmer_factor = (double) channels[DIMMER] / 255.0;
	uint8_t selected_gobo;

	//floating point error
	if(scaling_factor > 1.0){
		scaling_factor = 1.0;
	}

	//gobo fallback selection
	for(selected_gobo = channels[GOBO]; !(xres->gobo[selected_gobo].data) && selected_gobo >= 0; selected_gobo--){
	}

	glBindFramebuffer(GL_FRAMEBUFFER, xres->fboID);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(xres->gobo_program_ID);
	glBindTexture(GL_TEXTURE_2D, xres->gobo_texture_ID);
	if(selected_gobo != xres->gobo_last){
		//glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
		xres->gobo_last = selected_gobo;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xres->gobo[xres->gobo_last].width, xres->gobo[xres->gobo_last].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, xres->gobo[xres->gobo_last].data);
		fprintf(stderr,"Changing gobo\n");
		//glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
	}

	//shutter implementation
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

	const double angle = M_PI * 2 *((double) (channels[ROTATION] / 255.0));
	const double cos_a = cos(angle);
	const double sin_a = sin(angle);
	const double win_scale = (double)xres->window_height/(double)xres->window_width;
	double window_x_scale = scaling_factor;
	double window_y_scale = scaling_factor;

	if( win_scale <= 1.0 ){
		window_x_scale *= win_scale;
	}else{
		window_y_scale /= win_scale;
	}

	double x_pos = (((double)(channels[PAN] << 8 | channels[PAN_FINE]) / 65535.0 * 2 * ( 1 - window_x_scale ))) - 1 + window_x_scale;
	double y_pos = (((double)(channels[TILT] << 8 | channels[TILT_FINE]) / 65535.0 * 2 * ( 1 - window_y_scale ))) - 1 + window_y_scale;
	float modelview[4][4] = 
	{ 
		{cos_a * window_x_scale,-sin_a * window_y_scale,0,0},
		{sin_a * window_x_scale,cos_a * window_y_scale,0,0},
		{0,0,1,0},
		{x_pos,y_pos,0,1}
	};

	glUniformMatrix4fv(xres->gobo_modelview_ID, 1, GL_FALSE, &(modelview[0][0]));
	glUniform1i(xres->gobo_program_texture_sampler, 0);
	glUniform4f(xres->gobo_program_colormod, channels[RED] * dimmer_factor / 255.0, channels[GREEN] * dimmer_factor / 255.0, channels[BLUE] * dimmer_factor /255.0, 0.0);
	glEnableVertexAttribArray(xres->gobo_program_attribute);
	glBindBuffer(GL_ARRAY_BUFFER, xres->fbo_vbo_ID);
	glVertexAttribPointer(
		xres->gobo_program_attribute,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableVertexAttribArray(xres->gobo_program_attribute);

	//Currently no need for filter
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(xres->fbo_program_ID);
	glBindTexture(GL_TEXTURE_2D, xres->fbo_texture);
	glUniform1i(xres->fbo_program_texture_sampler, 0);
	glEnableVertexAttribArray(xres->fbo_program_attribute);
	glBindBuffer(GL_ARRAY_BUFFER, xres->fbo_vbo_ID);
	glVertexAttribPointer(
		xres->fbo_program_attribute,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(xres->fbo_program_attribute);

	//swap buffers
	glXSwapBuffers(xres->display, xres->main);
	return 0;

}

void backend_free(XRESOURCES* res){
	glDeleteTextures(1, &res->fbo_texture);
	glDeleteRenderbuffers(1, &res->rbo_depth);
	glDeleteFramebuffers(1, &res->fboID);
	glDeleteTextures(1, &res->gobo_texture_ID);
	glDeleteBuffers(1, &res->fbo_vbo_ID);
	glDeleteProgram(res->gobo_program_ID);
	glDeleteProgram(res->fbo_program_ID);
	glXDestroyContext(res->display, res->gl_context);
}
