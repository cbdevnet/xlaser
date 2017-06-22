int backend_compile_shader(GLuint shader_id, unsigned char** shader_source, ssize_t shader_length){
	GLint result = GL_FALSE;
	int log_length;
	char* log = NULL;

	glShaderSource(shader_id, 1, (const GLchar**)shader_source, (const GLint*)&shader_length);
	glCompileShader(shader_id);

	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
	if(result != GL_TRUE){
		fprintf(stderr, "Failed to compile shader\n");
	}

	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 1){ //contrary to the docs, this returns a single newline character upon success (https://www.khronos.org/opengles/sdk/docs/man/xhtml/glGetShaderiv.xml)
		log = calloc(log_length + 1, sizeof(char));
		glGetShaderInfoLog(shader_id, log_length, NULL, log);
		fprintf(stderr, "Shader compiler error: %s\n", log);
		free(log);
		return 1;
	}
	return 0;
}

GLuint backend_compile_program(unsigned char* vertex_source, ssize_t vertex_length, unsigned char* fragment_source, ssize_t fragment_length){
	//create two shader handles and a program handle
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint program_id = glCreateProgram();

	//compile the shaders
	if(backend_compile_shader(vertex_shader, &vertex_source, vertex_length)
			|| backend_compile_shader(fragment_shader, &fragment_source, fragment_length)){
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
	unsigned u;
	int major = 0, minor = 0;
	GLenum glew_err;

	GLfloat target_quad[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1
	};

	//create gl context
	res->backend.gl_context = glXCreateContext(res->display, &(res->visual_info), NULL, True);
	if(!(res->backend.gl_context)){
		fprintf(stderr, "Failed to create GL context\n");
		return -1;
	}

	glXMakeContextCurrent(res->display, res->main, res->main, res->backend.gl_context);
	glXQueryVersion(res->display, &major, &minor);
	fprintf(stderr, "OpenGL version %d.%d\n", major, minor);

	//glewExperimental = GL_TRUE;
	glew_err = glewInit();
	if(glew_err != GLEW_OK){
		fprintf(stderr, "Failed to initialize glew: %s\n", glewGetErrorString(glew_err));
		return -2;
	}

	printf("OpenGL implementation: %s\n", glGetString(GL_VERSION));

	glClearColor(0.f, 0.f, 0.f, 0.f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    	//Create VAO and ignore it
    	GLuint vertexArrayID;
    	glGenVertexArrays(1, &vertexArrayID);
    	glBindVertexArray(vertexArrayID);

	//enable some features
    	glEnable(GL_TEXTURE_2D);
    	glEnable(GL_MULTISAMPLE);
    	glEnable(GL_BLEND);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//render target
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(3, res->backend.fbo_texture);
	glGenFramebuffers(3, res->backend.fboID);
	glGenRenderbuffers(3, res->backend.rbo_depth);

	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);

	//set up textures
	for(u = 0; u < 3; u++){
		glBindTexture(GL_TEXTURE_2D, res->backend.fbo_texture[u]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//generate framebuffers and attach renderbuffers
		glBindFramebuffer(GL_FRAMEBUFFER, res->backend.fboID[u]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res->backend.fbo_texture[u], 0);

		glBindRenderbuffer(GL_RENDERBUFFER, res->backend.rbo_depth[u]);
		if(u < 2){
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, res->window_width * 0.5, res->window_height * 0.5);
		}else{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, res->window_width, res->window_height);
		}
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, res->backend.rbo_depth[u]);
		
		if(u < 2){
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, res->window_width * 0.5, res->window_height * 0.5, 0, GL_RGB, GL_FLOAT, 0L);
		}else{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, res->window_width, res->window_height, 0, GL_RGB, GL_FLOAT, 0L);
		}
	}

	//unbind everything
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//generate an arraybuffer. since this is the only one, it can stay bound
	glGenBuffers(1, &res->backend.fbo_vbo_ID);
	glBindBuffer(GL_ARRAY_BUFFER, res->backend.fbo_vbo_ID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(target_quad), target_quad, GL_STATIC_DRAW);

	//gobo texture texture
	glGenTextures(1, &res->backend.gobo_texture_ID);
	glBindTexture(GL_TEXTURE_2D, res->backend.gobo_texture_ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//bind gobo texture in order to upload data
	glBindTexture(GL_TEXTURE_2D, 0);

	//generate gobo shader program
	res->backend.gobo_last = -1;
	res->backend.update_last_frame = true;
	fprintf(stderr, "Compiling gobo shader\n");
	res->backend.program[0].id = backend_compile_program(gobo_vertex_shader, gobo_vertex_shader_len, gobo_fragment_shader, gobo_fragment_shader_len);
	res->backend.program[0].sampler[0] = glGetUniformLocation(res->backend.program[0].id, "textureSampler");
	res->backend.program[0].attribute = glGetAttribLocation(res->backend.program[0].id, "vertexCoord");
	res->backend.program[0].modelview = glGetUniformLocation(res->backend.program[0].id, "modelview");

	//generate blur shader program
	fprintf(stderr, "Compiling blur effect shader\n");
	res->backend.program[1].id = backend_compile_program(filter_vertex_shader, filter_vertex_shader_len, filter_fragment_shader, filter_fragment_shader_len);
	res->backend.program[1].sampler[0] = glGetUniformLocation(res->backend.program[1].id, "textureSampler");
	res->backend.program[1].horizontal = glGetUniformLocation(res->backend.program[1].id, "horizontal");
	res->backend.program[1].exposure = glGetUniformLocation(res->backend.program[1].id, "goboSize");
	res->backend.program[1].attribute = glGetAttribLocation(res->backend.program[1].id, "vertexCoord");
	res->backend.program[1].modelview = glGetUniformLocation(res->backend.program[1].id, "modelview");

	//generate hdr shader program
	fprintf(stderr, "Compiling HDR shader\n");
	res->backend.program[2].id = backend_compile_program(hdr_vertex_shader, hdr_vertex_shader_len, hdr_fragment_shader, hdr_fragment_shader_len);
	res->backend.program[2].sampler[0] = glGetUniformLocation(res->backend.program[2].id, "textureSampler" );
	res->backend.program[2].sampler[1] = glGetUniformLocation(res->backend.program[2].id, "goboSampler" );
	res->backend.program[2].exposure = glGetUniformLocation(res->backend.program[2].id, "exposure" );
	res->backend.program[2].attribute = glGetAttribLocation(res->backend.program[2].id, "vertexCoord" );
	res->backend.program[2].color = glGetUniformLocation(res->backend.program[2].id, "colormod" );
	return 0;
}

int xlaser_reconfigure(XRESOURCES* xres){
	unsigned u;
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};

	//resize renderbuffers
	for(u = 0; u < 3; u++){
		glBindRenderbuffer(GL_RENDERBUFFER, xres->backend.rbo_depth[u]);
		glBindFramebuffer(GL_FRAMEBUFFER, xres->backend.fboID[u]);
		
		glBindRenderbuffer(GL_RENDERBUFFER, xres->backend.rbo_depth[u]);
		if(u < 2){
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, xres->window_width * 0.5, xres->window_height * 0.5);
		}else{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, xres->window_width, xres->window_height);
		}
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, xres->backend.rbo_depth[u]);
		glBindTexture(GL_TEXTURE_2D, xres->backend.fbo_texture[u]);
		
		if(u < 2){
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, xres->window_width * 0.5, xres->window_height * 0.5, 0, GL_RGB, GL_FLOAT, 0L);
		}else{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, xres->window_width, xres->window_height, 0, GL_RGB, GL_FLOAT, 0L);
		}
	}

	glDrawBuffers(1, drawBuffers);

	//unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//update viewport and request re-render
	glViewport(0, 0, xres->window_width, xres->window_height);
	xres->backend.update_last_frame = true;
	return 0;
}

int xlaser_render(XRESOURCES* xres, uint8_t* channels){
	unsigned u;
	double scaling_factor = (double)(256 - channels[ZOOM]) / 255.0;
	long shutter_offset = 0, shutter_interval = 0;
	double dimmer_factor = (double) channels[DIMMER] / 255.0;
	uint8_t selected_gobo;
	double focus = (double) channels[FOCUS] / 255.0;
	const double angle = M_PI * 2 *((double) (channels[ROTATION] / 255.0));
	const double cos_a = cos(angle);
	const double sin_a = sin(angle);
	const double win_scale = (double)xres->window_height / (double)xres->window_width;
	const double window_x_scale = scaling_factor * ((win_scale <= 1.0) ? win_scale : 1.0);
	const double window_y_scale = scaling_factor / ((win_scale > 1.0) ? win_scale : 1.0);
	double x_pos = (((double)(channels[PAN] << 8 | channels[PAN_FINE]) / 65535.0 * 2 * ( 1 - window_x_scale ))) - 1 + window_x_scale;
	double y_pos = (((double)(channels[TILT] << 8 | channels[TILT_FINE]) / 65535.0 * 2 * ( 1 - window_y_scale ))) - 1 + window_y_scale;

	float modelview[4][4] = {
		{cos_a * window_x_scale, -sin_a * window_y_scale, 0, 0},
		{sin_a * window_x_scale, cos_a * window_y_scale, 0, 0},
		{0, 0, 1, 0},
		{x_pos, y_pos, 0, 1}
	};

	//int pongs = (int) channels[FOCUS] / 12.0;
	const unsigned pongs = 2;

	if(focus > 1.0){
		focus = 1.0;
	}
	focus = (exp(focus) - 1.0) * 10;

	//floating point error
	if(scaling_factor > 1.0){
		scaling_factor = 1.0;
	}

	//gobo fallback selection
	for(selected_gobo = channels[GOBO]; selected_gobo >= 0 && !(xres->gobo[selected_gobo].data); --selected_gobo){}

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

	//bind and load gobo texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, xres->backend.gobo_texture_ID);
	if(selected_gobo != xres->backend.gobo_last){
		xres->backend.gobo_last = selected_gobo;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xres->gobo[xres->backend.gobo_last].width, xres->gobo[xres->backend.gobo_last].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, xres->gobo[xres->backend.gobo_last].data);
		fprintf(stderr, "Changing gobo\n");
		xres->backend.update_last_frame = true;
	}

	//actually draw
	if(true){
		glViewport(0, 0, xres->window_width * 0.5, xres->window_height * 0.5);
		xres->backend.update_last_frame = false;
		
		//lightmap to framebuffer 0
		glBindFramebuffer(GL_FRAMEBUFFER, xres->backend.fboID[0]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(xres->backend.program[0].id);
		glBindTexture(GL_TEXTURE_2D, xres->backend.gobo_texture_ID);

		glUniform1i(xres->backend.program[0].sampler[0], 0);
		glUniform3f(xres->backend.program[0].color, 1.0, 1.0, 1.0);
		glUniformMatrix4fv(xres->backend.program[0].modelview, 1, GL_FALSE, modelview[0]);
		
		glEnableVertexAttribArray(xres->backend.program[0].attribute);
		glBindBuffer(GL_ARRAY_BUFFER, xres->backend.fbo_vbo_ID);
		glVertexAttribPointer(xres->backend.program[0].attribute, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(xres->backend.program[0].attribute);

		//Blur/Filter program ping pong 0 to 1 then to 0
		glUseProgram(xres->backend.program[1].id);
		glUniform1i(xres->backend.program[1].sampler[0], 0);
		glUniform2f(xres->backend.program[1].exposure, xres->window_width * 0.5, xres->window_height * 0.5);
		glUniformMatrix4fv(xres->backend.program[1].modelview, 1, GL_FALSE, modelview[0]);

		glEnableVertexAttribArray(xres->backend.program[1].attribute);
		glBindBuffer(GL_ARRAY_BUFFER, xres->backend.fbo_vbo_ID);
		glVertexAttribPointer(xres->backend.program[1].attribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
		for(u = 0; u < pongs; u++){
			glBindFramebuffer(GL_FRAMEBUFFER, xres->backend.fboID[1]);
			glBindTexture(GL_TEXTURE_2D, xres->backend.fbo_texture[0]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUniform1i( xres->backend.program[1].horizontal, 0);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindFramebuffer(GL_FRAMEBUFFER, xres->backend.fboID[0]);
			glBindTexture(GL_TEXTURE_2D, xres->backend.fbo_texture[1]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUniform1i( xres->backend.program[1].horizontal, 1 );
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		glDisableVertexAttribArray(xres->backend.program[1].attribute);

		//render the gobo
		glViewport(0, 0, xres->window_width, xres->window_height);
		
		glBindFramebuffer(GL_FRAMEBUFFER, xres->backend.fboID[2]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(xres->backend.program[0].id);
		glBindTexture(GL_TEXTURE_2D, xres->backend.gobo_texture_ID);

		glUniformMatrix4fv(xres->backend.program[0].modelview, 1, GL_FALSE, modelview[0]);
		glUniform1i(xres->backend.program[0].sampler[0], 0);
		glUniform3f(xres->backend.program[0].color, 1.0, 1.0, 1.0);
		glEnableVertexAttribArray(xres->backend.program[0].attribute);
		glBindBuffer(GL_ARRAY_BUFFER, xres->backend.fbo_vbo_ID);
		glVertexAttribPointer(xres->backend.program[0].attribute, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(xres->backend.program[0].attribute);

	}

	//run hdr program
	glBindTexture(GL_TEXTURE_2D, xres->backend.fbo_texture[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, xres->backend.fbo_texture[2]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(xres->backend.program[2].id);
	glUniform1i(xres->backend.program[2].sampler[0], 0);
	glUniform1i(xres->backend.program[2].sampler[1], 1);
	glUniform1f(xres->backend.program[2].exposure, focus);
	glUniform3f(xres->backend.program[2].color, channels[RED] * dimmer_factor / 255.0, channels[GREEN] * dimmer_factor / 255.0, channels[BLUE] * dimmer_factor /255.0);
	glUniformMatrix4fv(xres->backend.program[2].modelview, 1, GL_FALSE, modelview[0]);

	glEnableVertexAttribArray(xres->backend.program[2].attribute);
	glBindBuffer(GL_ARRAY_BUFFER, xres->backend.fbo_vbo_ID);
	glVertexAttribPointer(xres->backend.program[2].attribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(xres->backend.program[2].attribute);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	//swap back and front buffers
	glXSwapBuffers(xres->display, xres->main);
	if(channels[TRACE] <= 100){
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}else{
		
	}
	return 0;
}

void backend_free(XRESOURCES* res){
	unsigned u;
	glDeleteTextures(2, res->backend.fbo_texture);
	glDeleteRenderbuffers(2, res->backend.rbo_depth);
	glDeleteFramebuffers(2, res->backend.fboID);

	glDeleteTextures(1, &(res->backend.gobo_texture_ID));
	glDeleteBuffers(1, &(res->backend.fbo_vbo_ID));
	for(u = 0; u < sizeof(res->backend.program) / sizeof(PROGRAM); u++){
		glDeleteProgram(res->backend.program[u].id);
	}

	glXDestroyContext(res->display, res->backend.gl_context);
}
