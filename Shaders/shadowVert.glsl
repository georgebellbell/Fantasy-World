#version 330 core
uniform mat4 modelMatrix;
layout (std140) uniform Matrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
    mat4 shadowMatrix;
};

in vec3 position;

void main(void) {
    gl_Position = (projMatrix * viewMatrix * modelMatrix) * vec4(position, 1.0);
}
