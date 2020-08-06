#version 330 core
in vec3 WorldPosition;
in vec2 TexCoords;
in vec3 Normal;

out vec4 FragColor;

struct Light{
    vec3 position;
    vec3 color;
};
uniform Light mainLight;

struct Material{
    vec3 albedo;
    float metallic;
    float roughness;
};
uniform Material material;

uniform mat4 worldToLight;

void main()
{

}