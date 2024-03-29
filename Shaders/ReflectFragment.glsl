#version 330 core
#extension GL_ARB_bindless_texture : enable

layout(bindless_sampler) uniform sampler2D bindless;
uniform textures{
    sampler2D arr[24];
} terrain;

uniform samplerCube cubeTex;


uniform light{
    vec4 lightColour;
    vec3 lightPosition;
    float lightRadius;
    vec4 lightSpecular;
};

uniform camera{
  vec3 cameraPos;
};

in Vertex{
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
	vec3 worldPos;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;

void main(void) {
    vec3 incident = normalize(lightPosition - IN.worldPos);
	vec3 viewDir = normalize(cameraPos- IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

    vec4 diffuse = texture(terrain.arr[17], IN.texCoord);
    vec3 bumpNormal = texture(terrain.arr[18], IN.texCoord).rgb;
    
    bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));
    float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPosition - IN.worldPos);
	float attenuation = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

    float specFactor = clamp(dot(halfDir,bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0f);

    vec3 reflectDir = reflect(-viewDir, normalize(bumpNormal));

    
    gPosition = vec4(IN.worldPos, 1.0);

    vec4 diffuseReflect = texture(cubeTex, reflectDir) + diffuse * 0.25;
    
    vec3 surface = diffuseReflect.rgb * lightColour.rgb;

    gColour.rgb = surface * lambert * attenuation;
	gColour.rgb += (lightSpecular.rgb * specFactor)*attenuation*0.33;
	gColour.rgb += surface * 0.1f;
	gColour.a = 0.7;



 


}