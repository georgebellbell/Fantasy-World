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

in Vertex {
    vec2 texCoord;
    float heightNormal;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
	vec3 worldPos;
    vec4 shadowProj;

} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;

vec4 get_texture_value(int offset){
    vec4 snow = texture(terrain.arr[0 + offset], IN.texCoord);
    vec4 rock = texture(terrain.arr[2 + offset], IN.texCoord);
    vec4 grass = texture(terrain.arr[4 + offset], IN.texCoord);
    vec4 sand = texture(terrain.arr[6 + offset], IN.texCoord);
    
    float bufferRange = 0.05;
    
    if (IN.heightNormal <= 0.2){
        float grassBias = max((IN.heightNormal - 0.15) / bufferRange, 0);
        float sandBias = 1 - grassBias;

        return vec4((sand * sandBias + grass * grassBias).rgb, 1.0);
    }
    else if(IN.heightNormal <= 0.3 ){
        float rockBias = max((IN.heightNormal - 0.25) / bufferRange, 0);
        float grassBias = 1 - rockBias;

        return vec4((grass * grassBias + rock * rockBias).rgb, 1.0);
    }
    else{
        float snowBias = max((IN.heightNormal - 0.65) / bufferRange, 0);
        float rockBias = 1 - snowBias;

        return vec4((rock * rockBias + snow * snowBias).rgb, 1.0);
    }
}
void main(void) {

    vec3 incident = normalize(lightPosition - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

    vec4 diffuse = get_texture_value(0);
    vec3 bumpNormal = get_texture_value(1).rgb;

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

    