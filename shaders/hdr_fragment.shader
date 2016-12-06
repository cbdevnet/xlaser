#version 120
varying vec2 UV;

uniform sampler2D textureSampler;
uniform sampler2D goboSampler;
uniform float exposure = 1.0;

void main(){
	const float gamma = 2.2;
	vec3 hdrColor = texture2D( goboSampler, UV ).rgb;
	vec3 bloomColor = texture2D( textureSampler, UV ).rgb;
	//hdrColor += bloomColor;
	vec3 result = vec3(1.0) - exp( -bloomColor * exposure -hdrColor );
	result = pow( result, vec3( 1.0 / gamma ));
	gl_FragColor = vec4( result, 1.0 );
}
