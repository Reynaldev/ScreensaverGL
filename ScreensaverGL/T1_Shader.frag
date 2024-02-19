#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture = false;

uniform vec3 uColor;

void main()
{
	if (uUseTexture)
		fragColor = texture(uTexture, texCoord) * vec4(uColor, 1.0);
	else
		fragColor = vec4(uColor, 1.0);
}