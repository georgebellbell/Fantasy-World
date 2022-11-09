#version 330 core
#extension GL_ARB_bindless_texture : enable
uniform sampler2D diffuseTex;
uniform sampler2D snowTex;
uniform sampler2D grassTex;

//uniform sampler2D sandTex;

float blendingFactor = 5.0;

layout(bindless_sampler) uniform sampler2D bindless;
uniform textures{
    sampler2D arr[1];
} terrain;

in Vertex {
    vec2 texCoord;
    float heightNormal;
} IN;



out vec4 fragColour;
void main(void) {
   

    vec4 snowColour = texture(snowTex, IN.texCoord);
    vec4 rockColour = texture(diffuseTex, IN.texCoord);
    vec4 grassColour = texture(grassTex, IN.texCoord);
    vec4 sandColour = texture(terrain.arr[0], IN.texCoord);
    
    float bufferRange = 0.05;
    
    

    if (IN.heightNormal <= 0.2){
        float grassBias = max((IN.heightNormal - 0.15) / bufferRange, 0);
        float sandBias = 1 - grassBias;

        fragColour = vec4((sandColour * sandBias + grassColour * grassBias).rgb, 1.0);
    }
    else if(IN.heightNormal <= 0.3 ){
        float rockBias = max((IN.heightNormal - 0.25) / bufferRange, 0);
        float grassBias = 1 - rockBias;

        fragColour = vec4((grassColour * grassBias + rockColour * rockBias).rgb, 1.0);
    }
    else{
        float snowBias = max((IN.heightNormal - 0.65) / bufferRange, 0);
        float rockBias = 1 - snowBias;

        fragColour = vec4((rockColour * rockBias + snowColour * snowBias).rgb, 1.0);
    }
}