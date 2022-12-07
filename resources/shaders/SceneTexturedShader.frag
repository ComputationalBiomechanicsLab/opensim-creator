#version 330 core

uniform bool uHasShadowMap = false;
uniform sampler2D uDiffuseTexture;
uniform vec3 uLightDir;
uniform sampler2D uShadowMapTexture;
uniform float uAmbientStrength;
uniform vec3 uLightColor;
uniform float uNear;
uniform float uFar;

in vec3 FragWorldPos;
in vec4 FragLightSpacePos;
in vec3 NormalWorldDir;
in float NonAmbientBrightness;
in vec2 TexCoord;

out vec4 Color0Out;

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
    float bias = max(0.025 * (1.0 - abs(dot(NormalWorldDir, uLightDir))), 0.0025);

    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMapTexture, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(uShadowMapTexture, projCoords.xy + vec2(x, y)*texelSize).r;
            if (pcfDepth < 1.0)
            {
                shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
            }
        }
    }
    shadow /= 9.0;

    return shadow;
}

float LinearizeDepth(float depth)
{
    // from: https://learnopengl.com/Advanced-OpenGL/Depth-testing
    //
    // only really works with perspective cameras: orthogonal cameras
    // don't need this unprojection math trick

    float z = depth * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

void main()
{
    float shadowAmt = uHasShadowMap ? 0.5*CalculateShadowAmount() : 0.0;
    float brightness = clamp(uAmbientStrength + ((1.0 - shadowAmt) * NonAmbientBrightness), 0.0, 1.0);
    Color0Out = vec4(brightness * uLightColor, 1.0) * texture(uDiffuseTexture, TexCoord);
    Color0Out.a *= 1.0 - (LinearizeDepth(gl_FragCoord.z) / uFar);  // fade into background at high distances
    Color0Out.a = clamp(Color0Out.a, 0.0, 1.0);
}