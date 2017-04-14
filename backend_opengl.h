#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

typedef struct /*PROGRAM IDs*/{
	GLuint id;
	GLuint attribute;
	GLuint sampler[2];
	GLuint color;
	GLuint modelview;
	GLuint horizontal;
	GLuint exposure;
}PROGRAM;

typedef struct /*_OPENGL_DATA*/ {
	GLXContext gl_context;
	GLuint fboID[2];
	GLuint rbo_depth[2];
	GLuint fbo_texture[2];
	GLuint fbo_vbo_ID;
	PROGRAM program[3];
	GLuint gobo_texture_ID;
	int gobo_last;
	bool update_last_frame;
} backend_data;
