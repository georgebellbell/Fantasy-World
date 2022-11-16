#version 330 core
#extension GL_ARB_bindless_texture : enable

layout(bindless_sampler) uniform sampler2D bindless;
uniform textures{
    sampler2D arr[24];
} terrain;

uniform light{
    vec4 lightColour;
    vec3 lightPosition;
    float lightRadius;
    vec4 lightSpecular;
};
uniform camera{
  vec3 cameraPos;
};
uniform int textureIndex;

in Vertex {
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void){
    vec3 incident = normalize(lightPosition - IN.worldPos);
	vec3 viewDir = normalize(cameraPos- IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

    vec4 diffuse = texture(terrain.arr[textureIndex], IN.texCoord);
    vec3 bumpNormal = texture(terrain.arr[textureIndex + 1], IN.texCoord).rgb;
    bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));
   
	float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPosition - IN.worldPos);
	float attenuation = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir,bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0f);

	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightSpecular.rgb * specFactor)*attenuation*0.33;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = diffuse.a;

    
    
}