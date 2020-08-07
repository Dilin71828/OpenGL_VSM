#version 330 core
in vec3 WorldPosition;
in vec2 TexCoords;
in vec3 Normal;

out vec4 FragColor;

struct Light{
    vec3 position;
    vec3 intensity;
    float constant;
    float linear;
    float quadratic;
};
uniform Light mainLight;

struct Material{
    vec3 albedo;
    float metallic;
    float roughness;
};
uniform Material material;

uniform vec3 cameraPosition;
uniform mat4 worldToLight;
uniform float nearPlane;
uniform float farPlane;
uniform sampler2D varianceShadowMap;

float calculateShadow(float depth, vec2 uv){
    vec2 varianceData = texture(varianceShadowMap, uv).rg;
    float var = varianceData.g - varianceData.r*varianceData.r;
    if(depth <= varianceData.r){
        return 1.0;
    }
    else{
        return var/(var+pow(depth-varianceData.r, 2.0));
    }
}

float linearizeDepth(float depth){
    float z = (2.0*nearPlane*farPlane)/(farPlane+nearPlane-depth*(farPlane-nearPlane));
    return (z-nearPlane)/(farPlane-nearPlane);
}

void main()
{
    // now we use phone lighting
    vec3 ambient = vec3(0.1);

    vec3 lightDirection = mainLight.position - WorldPosition;
    float dist = length(lightDirection);
    float attenuation = 1.0/(mainLight.constant + mainLight.linear * dist + mainLight.quadratic*dist*dist);
    lightDirection = normalize(lightDirection);
    vec3 viewDirection = normalize(cameraPosition-WorldPosition);
    //FragColor = vec4(viewDirection, 1.0);

    float NdotL = max(dot(Normal, lightDirection), 0.0);
    vec3 diffuse = NdotL * material.albedo * mainLight.intensity * attenuation;
    //FragColor = vec4(vec3(NdotL), 1.0);
    //FragColor = vec4(diffuse, 1.0);

    vec4 lightSpacePosition = worldToLight * vec4(WorldPosition, 1.0);
    lightSpacePosition.xyz=lightSpacePosition.xyz/lightSpacePosition.w;
    float depth = linearizeDepth(lightSpacePosition.z);
    lightSpacePosition = lightSpacePosition*0.5 + 0.5;
    float shadow = calculateShadow(depth, lightSpacePosition.xy);
    //FragColor = vec4(vec3(shadow), 1.0);
    diffuse *= shadow;

    vec3 color = ambient + diffuse;
    color = pow(color, vec3(1/2.2));
    FragColor = vec4(color, 1.0);
}