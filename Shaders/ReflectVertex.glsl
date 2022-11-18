#version 330 core

layout (std140) uniform Matrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
    mat4 shadowMatrix;
};
uniform mat4 modelMatrix;

uniform mat4 textureMatrix;

in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 tangent;

out Vertex {
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
} OUT;

void main(void) {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0);


    OUT.texCoord = texCoord;

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    vec3 wNormal = normalize(normalMatrix * normalize(normal));
    vec3 wTangent = normalize(normalMatrix * normalize(tangent.xyz));

    OUT.normal = -wNormal;
    OUT.tangent = wTangent;
    OUT.binormal = cross(wTangent, wNormal) * tangent.w;

    vec4 worldPos = (modelMatrix * vec4(position, 1));

    OUT.worldPos = worldPos.xyz;
}