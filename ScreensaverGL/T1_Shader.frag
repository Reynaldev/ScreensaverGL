#version 330 core

in vec3 color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture = false;

void main()
{
	if (uUseTexture)
		fragColor = texture(uTexture, texCoord) * vec4(color, 1.0);
	else
		fragColor = vec4(color, 1.0);
}