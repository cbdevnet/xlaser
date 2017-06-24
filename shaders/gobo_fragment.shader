#version 120
varying vec2 UV;
uniform sampler2D textureSampler;
uniform vec4 colormod = vec4(1.0,1.0,1.0,1.0);
void main(){
	
	vec4 textureColor = texture2D( textureSampler, UV ) * colormod;
	gl_FragColor = textureColor;

}
