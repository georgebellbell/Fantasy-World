#version 330 core
uniform samplerCube cubeTex;

in Vertex {
    vec3 viewDir;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;

void main(void) {
    gColour = texture(cubeTex, normalize(IN.viewDir));
}