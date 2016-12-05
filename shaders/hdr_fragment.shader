#version 120
varying vec2 UV;

uniform sampler2D textureSampler;
uniform float exposure = 1.0;

void main(){
	const float gamma = 2.2;
	vec3 hdrColor = texture2D( textureSampler, UV ).rgb * 2;
	vec3 result = vec3(1.0) - exp( -hdrColor * exposure );
	result = pow( result, vec3( 1.0 / gamma ));
	gl_FragColor = vec4( result, 1.0 );
}
