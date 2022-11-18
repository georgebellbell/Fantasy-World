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
    vec4 shadowProj;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;
//out vec4 fragColour;

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

    float shadow = 1.0;

    vec3 shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
    if( abs(shadowNDC.x) < 1.0f && 
        abs(shadowNDC.y) < 1.0f && 
        abs(shadowNDC.z) < 1.0f){
        vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
        float shadowZ = texture(terrain.arr[20], biasCoord.xy).x;
        if(shadowZ <biasCoord.z) {
            shadow = 0.0f;
        }
    }



	vec3 surface = (diffuse.rgb * lightColour.rgb);
	gColour.rgb = surface * lambert * attenuation;
	gColour.rgb += (lightSpecular.rgb * specFactor)*attenuation*0.33;
    gColour.rgb *= shadow;
	gColour.rgb += surface * 0.1f;
	gColour.a = diffuse.a;

    gPosition = vec4(IN.worldPos, 1.0);
    
}