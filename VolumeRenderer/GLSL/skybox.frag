#version 330

in vec2 gUV;
uniform sampler2D kImage;

layout(location=0) out vec4 outColor;  // out for output

void main () {
	outColor.rgba = texture2D(kImage, gUV);
}
