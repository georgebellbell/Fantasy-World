#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform sampler2D shadowTex;

uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
uniform vec4 lightSpecular;

in Vertex {
	vec4 colour; 
	vec2 texCoord;
	vec3 normal;
    vec3 tangent;
    vec3 binormal;
	vec3 worldPos;
    vec4 shadowProj;
} IN;

out vec4 fragColour;

void main(void) {
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

	vec4 diffuse = texture(diffuseTex, IN.texCoord);
    vec3 bumpNormal = texture(bumpTex, IN.texCoord).rgb;
    bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));

	float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir,bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0f);

    float shadow = 1.0f;

    vec3 shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
    if(abs(shadowNDC.x) < 1.0f && abs(shadowNDC.y) < 1.0f && abs(shadowNDC.z) < 1.0f){
        vec3 biasCoords = shadowNDC * 0.5f + 0.5f;
        float shadowZ = texture(shadowTex, biasCoords.xy).x;
        if (shadowZ < biasCoords.z) shadow = 0.0f;
    }

	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragColour.rgb = surface * attenuation * lambert;
	fragColour.rgb += (lightSpecular.rgb * attenuation * lambert)*0.33;
    fragColour *= shadow;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = diffuse.a;
}