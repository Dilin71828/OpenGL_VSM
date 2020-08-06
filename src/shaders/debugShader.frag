#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D debugTexture;

void main()
{
    FragColor = vec4(texture(debugTexture, TexCoords).rgb, 1.0);
}