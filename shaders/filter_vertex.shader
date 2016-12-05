#version 120
attribute vec2 vertexCoord;
varying vec2 UV;
void main(){
	UV = (vertexCoord + 1.0) / 2.0;
	gl_Position = vec4(vertexCoord, 0, 1);
}
