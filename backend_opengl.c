GLuint backend_compile(unsigned char *vertex_source, unsigned char *fragment_source){
	
	GLint result = GL_FALSE;
	int InfoLength;

	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertex_shader_id, 1, (const GLchar**) &vertex_source, NULL);
	glCompileShader(vertex_shader_id);

	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &InfoLength);
	if(InfoLength > 0){
		char *string = calloc(InfoLength + 1, sizeof(char));
		glGetShaderInfoLog(vertex_shader_id, InfoLength, NULL, string);
		printf("%s\n", &string[0]);
		free(string);
	}

	glShaderSource(fragment_shader_id, 1, (const GLchar**) &fragment_source, NULL);
	glCompileShader(fragment_shader_id);

	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &InfoLength);
	if(InfoLength > 0){
		char *string = calloc(InfoLength + 1, sizeof(char));
		glGetShaderInfoLog(fragment_shader_id, InfoLength, NULL, string);
		printf("%s\n", &string[0]);
		free(string);
	}
	
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);

	glDetachShader(program_id, vertex_shader_id);
	glDetachShader(program_id, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
	return program_id;
}


int backend_init(XRESOURCES* res, CONFIG* config){
	res->gl_context = glXCreateContext(res->display, &(res->visual_info), NULL, True);
	int major = 0, minor = 0;

	if(res->gl_context == 0L){
		fprintf(stderr,"GLXContext is null" );
	}

	glXMakeContextCurrent(res->display, res->main, res->main, res->gl_context);
	glXQueryVersion( res->display, &major, &minor);
	fprintf(stderr, "Major: %d Minor: %d\n", major, minor);

	//glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK){
		fprintf(stderr, "%s\n", glewGetErrorString(err));
		return 1;
	}

	printf("GL VERSION: %s\n", glGetString(GL_VERSION) );

	glClearColor( 0.f, 0.f, 0.f, 1.f );
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    	//Create VAO and ignore it
    	GLuint VertexArrayID;
    	glGenVertexArrays(1, &VertexArrayID);
    	glBindVertexArray(VertexArrayID);

	//EnableTextures
    	glEnable(GL_TEXTURE_2D);
    	glEnable(GL_MULTISAMPLE);

    	//EnableBlending
    	glEnable(GL_BLEND);
    	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	//EnableDepth
	glEnable( GL_DEPTH_TEST);
	glDepthFunc( GL_LESS );	

	int scrX = 0, scrY = 0;
	//Target texture for render
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(2, res->fbo_texture);
	glGenFramebuffers(2, res->fboID );
	glGenRenderbuffers( 2, res->rbo_depth );
	for( int i = 0; i < 2; ++i ){

		glBindTexture( GL_TEXTURE_2D, res->fbo_texture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scrX, scrY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L );

		glBindFramebuffer( GL_FRAMEBUFFER, res->fboID[i] );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res->fbo_texture[i], 0);
		
		glBindRenderbuffer( GL_RENDERBUFFER, res->rbo_depth[i] );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, scrX, scrY );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, res->rbo_depth[i] );

	}

	printf("height: %d width: %d\n", scrX, scrY);

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);

	glBindFramebuffer( GL_FRAMEBUFFER , 0);
	glBindRenderbuffer( GL_RENDERBUFFER, res->rbo_depth[0] );
	//Quad to draw to
	GLfloat vert[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1
	};

	glGenBuffers(1, &res->fbo_vbo_ID);
	glBindBuffer(GL_ARRAY_BUFFER, res->fbo_vbo_ID );
	glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STATIC_DRAW);
	//We only use one Arraybuffer, so no need to unbind it.
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	//GOBO Texture

	glGenTextures(1, &res->gobo_texture_ID );
	glBindTexture( GL_TEXTURE_2D, res->gobo_texture_ID );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//GOBO DATA TO GPU
	//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, res->gobo[0].width, res->gobo[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, res->gobo[0].data );

	glBindTexture(GL_TEXTURE_2D, 0);

	
	//Get Program ID for Gauss Filter
	fprintf(stderr, "Filterprogram\n");
	res->fbo_program_ID = backend_compile(filter_vertex, filter_fragment);
	res->fbo_program_texture_sampler = glGetUniformLocation( res->fbo_program_ID, "textureSampler");
	res->fbo_program_horizontal = glGetUniformLocation( res->fbo_program_ID, "horizontal");
	res->fbo_program_attribute = glGetAttribLocation( res->fbo_program_ID, "vertexCoord");
	
	//Get Program ID for Gobo
	fprintf(stderr, "Goboprogram\n");
	res->gobo_last = 10;
	res->gobo_program_ID = backend_compile( gobo_vertex, gobo_fragment );
	res->gobo_program_texture_sampler = glGetUniformLocation( res->gobo_program_ID, "textureSampler");
	res->gobo_modelview_ID = glGetUniformLocation( res->gobo_program_ID, "modelview" );
	res->gobo_program_colormod = glGetUniformLocation( res->gobo_program_ID, "colormod" );	
	res->gobo_program_attribute = glGetAttribLocation( res->gobo_program_ID, "vertexCoord" );

	//Light Program
	fprintf(stderr, "Lightprogram\n");
	res->light_program_ID = backend_compile( light_vertex, light_fragment );
	res->light_program_texture_sampler = glGetUniformLocation( res->light_program_ID, "textureSampler");
	res->light_modelview_ID = glGetUniformLocation( res->light_program_ID, "modelview" );
	res->light_program_colormod = glGetUniformLocation( res->light_program_ID, "colormod" );
	res->light_program_attribute = glGetAttribLocation( res->light_program_ID, "vertexCoord" );
	return 0;
}

int xlaser_reconfigure(XRESOURCES* xres)
{
	for( int i = 0; i < 2; ++i ){
		
		glBindRenderbuffer( GL_RENDERBUFFER, xres->rbo_depth[i]);
		glBindTexture( GL_TEXTURE_2D, xres->fbo_texture[i]);
		glBindFramebuffer( GL_FRAMEBUFFER, xres->fboID[i] );
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xres->window_width, xres->window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, xres->window_width, xres->window_height );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, xres->rbo_depth[i] );
	
	}

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);
	
	glBindFramebuffer( GL_FRAMEBUFFER , 0);
	glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, xres->gobo[xres->gobo_last].width, xres->gobo[xres->gobo_last].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, xres->gobo[xres->gobo_last].data );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glViewport( 0, 0, xres->window_width, xres->window_height );
	return 0;
}

int xlaser_render(XRESOURCES* xres, uint8_t* channels)
{
	double scaling_factor = (double)(256 - channels[ZOOM]) / 255.0;
	if( scaling_factor > 1.0)
	{
		scaling_factor = 1.0;
	}
	double dimmer_factor = (double) channels[DIMMER] / 255.0;
	uint8_t selected_gobo;
	for(selected_gobo = channels[GOBO]; !(xres->gobo[selected_gobo].data) && selected_gobo >= 0; selected_gobo--)
	{
		
	}
	
	glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
	if( selected_gobo != xres->gobo_last )
	{
		xres->gobo_last = selected_gobo;

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, xres->gobo[xres->gobo_last].width, xres->gobo[xres->gobo_last].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, xres->gobo[xres->gobo_last].data );
		fprintf(stderr,"Changing gobo\n");
	}
	long shutter_offset = 0, shutter_interval = 0;
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

	if( xres->window_height < xres->window_width ){
		window_x_scale *= (double)xres->window_height/(double)xres->window_width;
	}else{
		window_y_scale *= (double)xres->window_width/(double)xres->window_height;
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
	glUseProgram( xres->gobo_program_ID );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUniformMatrix4fv( xres->gobo_modelview_ID, 1, GL_FALSE, &(modelview[0][0]));
	glUniform1i( xres->gobo_program_texture_sampler, 0 );
	glUniform4f( xres->gobo_program_colormod, channels[RED] * dimmer_factor / 255.0, channels[GREEN] * dimmer_factor / 255.0, channels[BLUE] * dimmer_factor /255.0, 0.0);
	glEnableVertexAttribArray( xres->gobo_program_attribute );
	glBindBuffer( GL_ARRAY_BUFFER, xres->fbo_vbo_ID );
	glVertexAttribPointer(
		xres->gobo_program_attribute,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	glDisableVertexAttribArray( xres->gobo_program_attribute );
	
	glUseProgram( xres->light_program_ID );
	glUniform1i( xres->light_program_texture_sampler, 0 );
	glUniformMatrix4fv( xres->light_modelview_ID, 1, GL_FALSE, &(modelview[0][0]));	
	glUniform4f( xres->gobo_program_colormod, channels[RED] * dimmer_factor / 255.0, channels[GREEN] * dimmer_factor / 255.0, channels[BLUE] * dimmer_factor /255.0, 0.0);

	glBindFramebuffer( GL_FRAMEBUFFER, xres->fboID[0] );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnableVertexAttribArray( xres->light_program_attribute );
	glBindBuffer( GL_ARRAY_BUFFER, xres->fbo_vbo_ID );
	glVertexAttribPointer(
		xres->light_program_attribute,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	glDisableVertexAttribArray( xres->light_program_attribute );
	
	glUseProgram( xres->fbo_program_ID );
	glUniform1i( xres->fbo_program_texture_sampler, 0 );
	glEnableVertexAttribArray( xres->fbo_program_attribute );
	glVertexAttribPointer(
		xres->fbo_program_attribute,	
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);

	for( int i = 0; i < 1; ++i ){
		glBindTexture( GL_TEXTURE_2D, xres->fbo_texture[0]);
		glBindFramebuffer( GL_FRAMEBUFFER, xres->fboID[1] );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUniform1i( xres->fbo_program_horizontal, 0 );	
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		glBindTexture( GL_TEXTURE_2D, xres->fbo_texture[1]);
		glBindFramebuffer( GL_FRAMEBUFFER, xres->fboID[0] );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUniform1i( xres->fbo_program_horizontal, 1 );	
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	}
	glBindTexture( GL_TEXTURE_2D, xres->fbo_texture[0] );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );	

	glDisableVertexAttribArray( xres->fbo_program_attribute );

	
	glXSwapBuffers( xres->display, xres->main );
	return 0;

}

void backend_free(XRESOURCES* res)
{
	glDeleteTextures( 2, res->fbo_texture );
	glDeleteRenderbuffers( 2, res->rbo_depth );
	glDeleteFramebuffers( 2, res->fboID );

	glDeleteTextures( 1, &res->gobo_texture_ID );
	glDeleteBuffers( 1, &res->fbo_vbo_ID );

	glDeleteProgram( res->gobo_program_ID );
	glDeleteProgram( res->fbo_program_ID );
	glDeleteProgram( res->light_program_ID );

	glXDestroyContext( res->display, res->gl_context );
	//XFreeGC(res->display, res->window_gc);
}
