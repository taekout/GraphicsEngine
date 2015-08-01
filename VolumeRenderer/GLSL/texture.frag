#version 130

precision highp float;

in vec3 fragNormal;
in vec2 fragUV;
in vec3 fragL;
#ifdef Blinn
in vec3 V;
#endif

out vec4 outColor;

uniform sampler2D imageTexSampler;

void main()
{
#ifdef Blinn
	vec3 R = normalize(2.f * fragNormal * dot(fragNormal, fragL) - fragL);
	vec3 H = normalize(fragNormal + V);
	float specularity = max( dot(fragNormal, H), 0.f );
#endif

	float cosTheta = clamp( dot(fragNormal, fragL), 0.f, 1.f );
	outColor.rgb = texture(imageTexSampler, fragUV).rgb;
	outColor = vec4( outColor.rgb * (cosTheta), 0.8 );
}

