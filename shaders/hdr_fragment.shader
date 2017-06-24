#version 120
varying vec2 UV;

uniform sampler2D textureSampler;
uniform sampler2D goboSampler;
uniform float exposure = 1.0;

void main(){
	const float gamma = 2.2;
	vec4 hdrColor = texture2D( goboSampler, UV );
	vec4 bloomColor = texture2D( textureSampler, UV );
	//hdrColor += bloomColor;
	vec3 result = vec3(1.0) - exp( (-hdrColor.rgb * 0.5 -bloomColor.rgb) * exposure );
	result = pow( result, vec3( 1.0 / gamma ));
	gl_FragColor = vec4(result.rgb, 1.0);
}
