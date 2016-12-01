#version 120
varying vec2 UV;
uniform sampler2D textureSampler;
uniform vec2 goboSize = vec2(1024.0,1024.0);
uniform float weight[10] = float[] (0.161054, 0.158968, 0.152766, 0.142606, 0.128753, 0.111566, 0.091490, 0.06904, 0.044814, 0.019461 );
uniform int horizontal = 0;
void main(){
	int n = 10;
	vec2 tex_offset = vec2(1,1)/goboSize;
	vec4 color = vec4(0,0,0,0);
	if( horizontal != 0 ){
		for(int i = 0; i < n; ++i){
			color += texture2D( textureSampler, (UV + vec2( tex_offset.x * i,0))) * weight[i];
			color += texture2D( textureSampler, (UV - vec2( tex_offset.x * i,0))) * weight[i];	
		}
	}else{
		for(int i = 0; i < n; ++i){
			color += texture2D( textureSampler, (UV + vec2( 0, tex_offset.x * i))) * weight[i];
			color += texture2D( textureSampler, (UV - vec2( 0, tex_offset.x * i))) * weight[i];	
		}
	};
	gl_FragColor = color;
}
