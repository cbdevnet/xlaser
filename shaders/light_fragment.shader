#version 120
varying vec2 UV;
uniform sampler2D textureSampler;
uniform vec4 colormod;
void main(){
	vec4 col = texture2D( textureSampler, UV );
	float brightness = dot( col.rgb, vec3(0.2126, 0.7152, 0.0722 ));
	if( brightness > 1.0 ){
		gl_FragColor = vec4( col.rgb, 1.0 ); 
	}
	gl_FragColor = vec4(1,0,0,0);
	//vec4 alpha = texture2D( textureSampler, UV );
	//gl_FragColor = vec4( alpha.w * colormod.xyz, alpha.w );
}
