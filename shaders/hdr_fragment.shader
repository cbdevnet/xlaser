#version 120
varying vec2 UV;

uniform sampler2D scene;
uniform sampler2D blur;
uniform float exposure = 1.0;

void main(){
	const float gamma = 2.2;
	vec3 hdrColor = texture2D( scene, UV ).rgb;
	vec3 bloomColor = texture2D( blur, UV ).rgb;
	hdrColor += bloomColor;
	vec3 result = vec3(1.0) - exp(-hdrColor * exposure );
	result = pow( result, vec3(1.0 / gamma ));
	gl_FragColor = vec4( result, 1.0 );
	gl_FragColor = vec4( 1,0,0,1);	
}
