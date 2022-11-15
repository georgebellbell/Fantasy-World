#version 400


layout (std140) uniform Matrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

uniform mat4 modelMatrix;


in vec3 position;
in vec2 texCoord;
in vec4 jointWeights;
in ivec4 jointIndices;

uniform mat4 joints[128];

out Vertex {
    vec2 texCoord;
    //float weight;
} OUT;

void main(void) {
    vec4 localPos = vec4(position, 1.0f);
    vec4 skelPos = vec4(0,0,0,0);
    //OUT.weight = 0;
    for (int i = 0; i < 4; ++i){
        int jointIndex = jointIndices[i];
        float jointWeight = jointWeights[i];

        skelPos += joints[jointIndex] * localPos * jointWeight;
        //OUT.weight += jointWeight;
    }
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(skelPos.xyz, 1.0);
    OUT.texCoord = texCoord; ///4;    
}