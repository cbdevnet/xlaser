#version 120
varying vec2 UV;
uniform sampler2D textureSampler;
uniform vec2 goboSize = vec2(1.0,1.0);
uniform float weight[3] = float[] (0.222338, 0.210431, 0.1784 );//, 0.142606, 0.128753, 0.111566, 0.091490, 0.06904, 0.044814, 0.019461 );
uniform int horizontal = 0;

void main(){
	const int n = 3;
	vec2 tex_offset = 1 / goboSize;
	vec4 color = texture2D( textureSampler, UV) * weight[0];

	if( horizontal != 0 ){
		for(int i = 1; i < n; ++i){
			color += texture2D( textureSampler, (UV + vec2( tex_offset.x * i,0))) * weight[i];
			color += texture2D( textureSampler, (UV - vec2( tex_offset.x * i,0))) * weight[i];
		}
	}else{
		for(int i = 1; i < n; ++i){
			color += texture2D( textureSampler, (UV + vec2( 0, tex_offset.y * i))) * weight[i];
			color += texture2D( textureSampler, (UV - vec2( 0, tex_offset.y * i))) * weight[i];
		}
	};

	gl_FragColor = vec4(color.rgb, 1.0);
}
