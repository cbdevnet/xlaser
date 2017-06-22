#version 120
attribute vec2 vertexCoord;
varying vec2 UV;
uniform mat4 modelview;
void main(){
	UV = (1.0 + vertexCoord ) / 2.0;
	gl_Position = modelview * vec4(vertexCoord, -1.0, 1.0);
}
