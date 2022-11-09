#version 330 core
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 textureMatrix;

uniform float terrainHeight;

in vec3 position;
in vec2 texCoord;

out Vertex {
    vec2 texCoord;
    float heightNormal;
} OUT;

void main(void) {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0);

    OUT.heightNormal = position.y / terrainHeight;

    // if (normalisedHeight <= 0.2){
    //     OUT.heightColour = vec4(0,0,1, 1);
    // }
    // else if(normalisedHeight > 0.2 && normalisedHeight < 0.5 ){
    //     OUT.heightColour = vec4(0,1,0, 1);
    // }
    // else{
    //     OUT.heightColour = vec4(1,1, 1, 1);

    // }

    // OUT.heightColour = OUT.heightColour * normalisedHeight;
    OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0, 1.0)).xy;
}