#version 120
#extension GL_ARB_texture_rectangle : enable
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2DRect texture0;     //it can be named in any way, GLSL just links it
uniform sampler2DRect texture_spectrum;

uniform float center_x;
uniform float center_y;

void main(){
    vec2 pos = gl_TexCoord[0].st;     
	
	
	// vec2 center = vec2(640.0, 360.0) / 2.0;
	vec2 center = vec2(center_x, center_y);
	
	float dist = distance( center, pos );	//built-in function for distance 
	
	//Antialiasing
	vec2 spectrumPos = vec2( dist / 7.0, 0.5 );
	vec4 col_spectrum =  texture2DRect(texture_spectrum, spectrumPos);	//spectrum values
	float spectrValue = col_spectrum.r;		//get spectrum value, normally in [0..1] 


	vec2 delta = pos - center;
	delta *= 1-2*spectrValue;
	vec2 posS = center + delta;

	vec4 color = texture2DRect(texture0, posS);

	gl_FragColor = color;
}

