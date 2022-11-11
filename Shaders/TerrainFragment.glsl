#version 330 core
#extension GL_ARB_bindless_texture : enable

layout(bindless_sampler) uniform sampler2D bindless;
uniform textures{
    sampler2D arr[8];
} terrain;


uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
uniform vec4 lightSpecular;
in Vertex {
    vec2 texCoord;
    float heightNormal;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColour;

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
    vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

    vec4 diffuse = get_texture_value(0);
    vec3 bumpNormal = get_texture_value(1).rgb;

    bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));

	float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir,bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0f);

	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightSpecular.rgb * specFactor)*attenuation*0.33;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = diffuse.a;
} 

    