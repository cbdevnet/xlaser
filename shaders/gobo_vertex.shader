#version 120
attribute vec2 vertexCoord;
varying vec2 UV;
void main(){
	UV = (1.0 + vertexCoord ) / 2.0;
	gl_Position = vec4(vertexCoord * 0.5, -1.0, 1.0);
}
