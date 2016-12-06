#version 120
varying vec2 UV;
uniform sampler2D textureSampler;
uniform vec3 colormod = vec3(1.0,1.0,1.0);
void main(){
	gl_FragColor = texture2D( textureSampler, UV ) + vec4(colormod, 0);
}
