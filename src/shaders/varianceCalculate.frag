#version 330 core
out vec2 FragColor;
in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform bool horizontal;

void main()
{
    vec2 tex_offset = 1.0 / textureSize(depthTexture, 0);
    vec2 result = texture(depthTexture, TexCoords).rg;
    if (horizontal)
    {
        for (int i=1; i<5; i++){
            result += texture(depthTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rg;
            result += texture(depthTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rg;
        }
    }
    else{
        for (int i=1; i<5; i++){
            result += texture(depthTexture, TexCoords + vec2(0.0, tex_offset.y*i)).rg;
            result += texture(depthTexture, TexCoords - vec2(0.0, tex_offset.y*i)).rg;
        }
    }
    result = result / 9.0;
    FragColor = result;
}