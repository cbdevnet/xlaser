#version 120
attribute vec2 vertexCoord;
varying vec2 UV;
uniform mat4 modelview;
void main(){
	UV = ( vec2( vertexCoord.x, -vertexCoord.y) + 1.0 ) / 2.0;
	gl_Position = modelview * vec4(vertexCoord * 2, -1.0, 1.0);
}
