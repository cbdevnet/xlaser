#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/gl.h>

#include <stdio.h>
#include <math.h>

#include "openglprogram.h"


int backend_init(XRESOURCES* res, CONFIG* config)
{


	res->gl_context = glXCreateContext(res->display, &(res->visual_info), NULL, True);

	if( res->gl_context == 0L)
	{
		fprintf(stderr,"GLXContext is null" );
	}

	glXMakeContextCurrent( res->display, res->main, res->main, res->gl_context );

	int major = 0, minor = 0;
	glXQueryVersion( res->display, &major, &minor);
	printf( "Major: %d Minor: %d\n", major, minor);

	//glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if( err != GLEW_OK )
	{
		fprintf(stderr, glewGetErrorString(err));
		fprintf(stderr, "\n");
		return 1;
	}

	printf("GL VERSION: %s\n", glGetString(GL_VERSION) );

	glClearColor( 0.f, 0.f, 0.f, 1.f );
    	glClear(GL_COLOR_BUFFER_BIT);

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

	int scrX = 958, scrY = 1078;
	//Target texture for render
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &res->fbo_texture);
	glBindTexture( GL_TEXTURE_2D, res->fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scrX, scrY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L );
	

	printf("height: %d width: %d\n", scrX, scrY);

	//RENDER TO TEXTURE

	//Generate Framebuffer and attach Renderbuffer
	glGenFramebuffers(1, &res->fboID );
	glBindFramebuffer(GL_FRAMEBUFFER, res->fboID);
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res->fbo_texture, 0);
	
	glGenRenderbuffers( 1, &res->rbo_depth );
	glBindRenderbuffer( GL_RENDERBUFFER, res->rbo_depth );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, scrX, scrY );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, res->rbo_depth );

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);

	glBindFramebuffer( GL_FRAMEBUFFER , 0);

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

	int gobo_width = 0, gobo_height = 0;
	unsigned u;

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
	res->fbo_program_ID = create_program(1);
	res->fbo_program_texture_sampler= glGetUniformLocation( res->fbo_program_ID, "textureSampler");
	//res->fbo_program_filter 	= glGetUniformLocation( res->fbo_program_ID, "filter");
	res->fbo_program_attribute	= glGetAttribLocation( res->fbo_program_ID, "vertexCoord");
	
	//Get Program ID for Gobo
	res->gobo_last = 10;
	res->gobo_program_ID 		 = create_program(0);
	res->gobo_program_texture_sampler= glGetUniformLocation( res->gobo_program_ID, "textureSampler");
	res->gobo_modelview_ID 		 = glGetUniformLocation( res->gobo_program_ID, "modelview" );
	res->gobo_program_colormod	 = glGetUniformLocation( res->gobo_program_ID, "colormod" );	
	res->gobo_program_attribute 	 = glGetAttribLocation( res->gobo_program_ID, "vertexCoord" );
	return 0;
}

int xlaser_render(XRESOURCES* xres, uint8_t* channels)
{

	double scaling_factor = 1.0;
	double dimmer_factor = (double) channels[DIMMER] / 255.0;
	uint8_t selected_gobo;
	for(selected_gobo = channels[GOBO]; !(xres->gobo[selected_gobo].data) && selected_gobo >= 0; selected_gobo--)
	{
		
	}
	
	glBindFramebuffer( GL_FRAMEBUFFER, xres->fboID );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram( xres->gobo_program_ID );
	glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
	if( selected_gobo != xres->gobo_last )
	{
		//glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
		xres->gobo_last = selected_gobo;

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, xres->gobo[xres->gobo_last].width, xres->gobo[xres->gobo_last].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, xres->gobo[xres->gobo_last].data );
		fprintf(stderr,"Changing gobo\n");
		//glBindTexture( GL_TEXTURE_2D, xres->gobo_texture_ID );
	}
	
	double angle = M_PI * 2 *((double) (channels[ROTATION] / 255.0));
	double cos_a = cos(angle);
	double sin_a = sin(angle);

	float modelview[4][4] = 
	{ 
		{cos_a,-sin_a,0,0},
		{sin_a,cos_a,0,0},
		{0,0,1,0},
		{0,0,0,1}
	};

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

	//glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableVertexAttribArray( xres->gobo_program_attribute );
	
	//Currently no need for filter

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram( xres->fbo_program_ID );
	glBindTexture( GL_TEXTURE_2D, xres->fbo_texture );
	glUniform1i( xres->fbo_program_texture_sampler, 0 );
	glEnableVertexAttribArray( xres->fbo_program_attribute );
	glBindBuffer( GL_ARRAY_BUFFER, xres->fbo_vbo_ID );
	glVertexAttribPointer(
		xres->fbo_program_attribute,	
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);
	
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	glDisableVertexAttribArray( xres->fbo_program_attribute );

	
	glXSwapBuffers( xres->display, xres->main );
	return 0;

}

void backend_free(XRESOURCES* res)
{
	glDeleteTextures( 1, &res->fbo_texture );

	glDeleteRenderbuffers( 1, &res->rbo_depth );

	glDeleteFramebuffers( 1, &res->fboID );

	glDeleteTextures( 1, &res->gobo_texture_ID );

	glDeleteBuffers( 1, &res->fbo_vbo_ID );

	glDeleteProgram( res->gobo_program_ID );
	glDeleteProgram( res->fbo_program_ID );

	glXDestroyContext( res->display, res->gl_context );
}
