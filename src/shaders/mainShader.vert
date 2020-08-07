#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec3 WorldPosition;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPosition = model * vec4(aPosition, 1.0);
    WorldPosition = vec3(worldPosition);
    Normal = mat3(transpose(inverse(model)))*aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * worldPosition;
}