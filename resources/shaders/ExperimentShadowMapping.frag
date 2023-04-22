#version 330 core

uniform vec3 uLightWorldPos;
uniform vec3 uViewWorldPos;
uniform sampler2D uDiffuseTexture;
uniform sampler2D uShadowMapTexture;

in vec3 FragWorldPos;
in vec4 FragLightSpacePos;
in vec3 NormalWorldDir;
in vec2 TexCoord;

out vec4 FragColor;

float CalculateShadowAmount()
{
    // perspective divide
    vec3 projCoords = FragLightSpacePos.xyz / FragLightSpacePos.w;

    // map to [0, 1]
    projCoords = 0.5*projCoords + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(uShadowMapTexture, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // calculate bias (based on depth map resolution and slope)
    vec3 normal = NormalWorldDir;
    vec3 lightDir = normalize(uLightWorldPos - FragWorldPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMapTexture, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(uShadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z >= 1.0-bias)
    {
        shadow = 0.0;
    }

    return shadow;
}

void main()
{
    vec3 frag2LightWorldDir = normalize(uLightWorldPos - FragWorldPos);

    float ambientAmt = 0.3;
    float diffuseAmt = max(dot(frag2LightWorldDir, NormalWorldDir), 0.0);
    float specularAmt = 0.0;
    {
        vec3 frag2ViewWorldDir = normalize(uLightWorldPos - FragWorldPos);
        vec3 halfwayDir = normalize(frag2LightWorldDir + frag2ViewWorldDir);
        specularAmt = pow(max(dot(NormalWorldDir, halfwayDir), 0.0), 64.0);
    }
    float shadowAmt = CalculateShadowAmount();

    float brightness = ambientAmt + ((1.0 - shadowAmt) * (diffuseAmt + specularAmt));
    vec3 diffuseColor = texture(uDiffuseTexture, TexCoord).rgb;

    FragColor = vec4(brightness * diffuseColor, 1.0);
}