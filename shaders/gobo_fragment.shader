#version 120
varying vec2 UV;
uniform sampler2D textureSampler;
uniform vec3 colormod = vec3(1.0,1.0,1.0);
void main(){
	vec4 textureColor = texture2D( textureSampler, UV );
		
	if( textureColor.w > 0.8  ) 
	gl_FragColor = textureColor + 1.0;
	else
	gl_FragColor = vec4(0,0,0,0);
}
