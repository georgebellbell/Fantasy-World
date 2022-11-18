#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D positionTex;

uniform float fogFactor;

uniform camera{
    vec3 cameraPos;
};

in Vertex{
    vec2 texCoord;
    vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void) {
    vec4 diffuse = texture(diffuseTex, IN.texCoord);
    vec3 position = texture(positionTex, IN.texCoord).rgb;
    float distanceToFrag = distance(cameraPos, position);

    float density = 0.0003 * fogFactor;

    float fog_factor_exp = clamp(1.0 - exp(-(pow(density * distanceToFrag,2))), 0, 1);
    vec4 fogColour = vec4(0.5,0.5,0.5,0.5);
    fragColour = mix(diffuse, fogColour, fog_factor_exp);


}