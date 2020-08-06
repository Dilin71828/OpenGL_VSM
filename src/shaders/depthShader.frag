#version 330 core
out vec2 FragColor;

uniform float nearPlane;
uniform float farPlane;

float linearizeDepth(float depth){
    float z = depth*2.0-1.0;
    return (2.0*nearPlane*farPlane)/(farPlane+nearPlane-z*(farPlane-nearPlane));
}

void main()
{
    float depth = linearizeDepth(gl_FragCoord.z);
    FragColor.x = (depth - nearPlane)/(farPlane-nearPlane);
    FragColor.y = FragColor.x*FragColor.x;
}