#include "openglprogram.h"

#include <GL/glew.h>
#include <GL/gl.h>

#include <stdio.h>

char *getGoboVertex()
{
	return "#version 120\n"
	"attribute vec2 vertexCoord;\n"
	"varying vec2 UV;\n"
	"uniform mat4 modelview;\n"
	"void main()\n"
	"{\n"
	"UV = (1.0 + vec2(vertexCoord.x, -vertexCoord.y) ) / 2.0;\n"
	"gl_Position = modelview * vec4(vertexCoord, 0, 1);\n"
	"}\n";
}

char *getGoboFragment()
{
	return "#version 120\n"
	"varying vec2 UV;\n"
	"uniform sampler2D textureSampler;\n"
	"uniform vec4 colormod;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D( textureSampler, UV ) + colormod;\n"
	"}\n";
}

char *getFilterVertex()
{
	return "#version 120\n"
	"attribute vec2 vertexCoord;\n"
	"varying vec2 UV;\n"
	"void main(){\n"
	"UV = (vertexCoord + 1.0) / 2.0;\n"
	"gl_Position = vec4(vertexCoord, 0, 1);\n"
	"}\n";
}

char *getFilterFragment()
{
return 	"#version 120\n"
	"varying vec2 UV;\n"
	"uniform sampler2D textureSampler;\n"
	"uniform float offset[5] = float[] (0.0, 1.0, 2.0, 3.0, 4.0);\n"
	"uniform float weight[10] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162, 0.01, 0.005, 0.0025, 0.00125, 0.000625 );\n"
	"void main(){\n"
		"vec4 color = vec4(0,0,0,0);\n"
		"for(int i = 1; i < 5; ++i)\n"
		"{\n"
			"for(int j = 1; j < 5; ++j){\n"
			"color += texture2D( textureSampler, (UV + vec2( offset[i], offset[j]) / 1024.0)) * weight[i+j];\n"
			"color += texture2D( textureSampler, (UV + vec2( -offset[i], -offset[j]) / 1024.0)) * weight[i+j];\n"
			"}\n"
		"}\n"
		"gl_FragColor = color;\n"
	"}\n";
}

GLuint create_program_with_string( char *vertex_source, char *fragment_source)
{

	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	GLint result = GL_FALSE;
	int InfoLength;

	glShaderSource(vertex_shader_id, 1, &vertex_source, NULL );
	glCompileShader(vertex_shader_id);

	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &InfoLength);
	if( InfoLength > 0)
	{
		//Error Log
		char *string = calloc( InfoLength + 1, sizeof(char) );
		glGetShaderInfoLog( vertex_shader_id, InfoLength, NULL, string );
		printf("%s\n", &string[0]);
		free( string );
	}

	glShaderSource(fragment_shader_id, 1, &fragment_source, NULL);
	glCompileShader( fragment_shader_id );
	//copy log stuff

	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result );
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &InfoLength);
	if( InfoLength > 0)
	{
		char *string = calloc( InfoLength + 1, sizeof( char ) );
		glGetShaderInfoLog( fragment_shader_id, InfoLength, NULL, string );
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

GLuint create_program( unsigned id )
{

	GLuint program_id;

	switch( id ){
	case 0:
		program_id = create_program_with_string( getGoboVertex(), getGoboFragment() );
		break;
	case 1:
		program_id = create_program_with_string( getFilterVertex(), getFilterFragment() );
		break;
	}

	return program_id;
}
