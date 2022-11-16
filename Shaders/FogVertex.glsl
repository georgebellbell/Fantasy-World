#version 330 core

layout (std140) uniform Matrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};
uniform mat4 modelMatrix;

in vec3 position;
in vec2 texCoord;

out Vertex {
    vec2 texCoord;
    vec3 worldPos;
} OUT;

void main(void) {
    OUT.texCoord = texCoord;
    vec4 worldPos = (modelMatrix * vec4(position, 1));
    OUT.worldPos = worldPos.rgb;
    gl_Position = (projMatrix * viewMatrix) * worldPos;
}