#version 330 core

layout (std140) uniform Matrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};
uniform mat4 textureMatrix;
uniform mat4 modelMatrix;

in vec3 position;
in vec2 texCoord;

out Vertex {
    vec2 texCoord;
} OUT;

void main(void) {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0);
    OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0, 1.0)).xy;
}