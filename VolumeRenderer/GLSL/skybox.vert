#version 330


layout(location=0) in vec4 inPositions;  // in for input
layout(location=1) in vec2 inUVs;

uniform mat4 Proj;
uniform mat4 View;

out vec2 gUV;  // g for global.

void main () {
	gUV = inUVs;
	gl_Position = Proj * View * inPositions;
}
